#include "Server.hpp"
#include <cstring>
#include <boost/signal.hpp>

boost::mutex logMutex;


Server::Server(int port, int numChannels, int tickInt) :
    mPort(port),
    mNumChannels(numChannels),
    mTickInterval(tickInt),
    mIOService(),
    mEndPoint(boost::asio::ip::tcp::v4(), mPort),
    mAcceptor(mIOService),  // , mEndPoint)  // Don't need to open() it later
    mStopRequested(false),
    mNewConnection(),
    mChan2Threads(),
    mConn2Threads()
{
    LOG( "In Server::Server()" << std::endl );
}


Server::~Server()
{
    LOG( "In Server::~Server()" << std::endl );

    if ( not mStopRequested )
        Stop();  // This should be done before we get here!
}


void Server::Start()
{
    LOG( "Server thread : " << boost::this_thread::get_id() << std::endl );

    // Create the channels
    try
    {
        for( int i=0; i<mNumChannels; ++i )
        {
            boost::shared_ptr<Channel> timer(new Channel(i+1, TICK_INTERVAL));
            boost::shared_ptr<boost::thread> thread(
                      new boost::thread(boost::bind(&Channel::Start, timer.get())));
            mChan2Threads[timer] = thread;
            LOG("Created timer " << (i+1) << ":" << timer.get() << " in a thread "
                 << thread.get() << ", id " << thread->get_id() << std::endl);
        }
    }
    catch(std::exception& e)
    {
        LOG( "Error creating channels: " << e.what() << std::endl );
        Stop();
        return;
    }

    // Listen for client connections
    try
    {
        mAcceptor.open(mEndPoint.protocol());
        mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        mAcceptor.bind(mEndPoint);
        mAcceptor.listen();
    }
    catch( boost::system::system_error& e )
    {
        LOG( "Error creating server listener: " << e.what() << std::endl );
        Stop();
        return;
    }

    /* How to deal with the clients? We can use Server's mIOService.
     * Then we can have a pool of Worker threads to do async work (read/write).
     * What about thread safety - another thread can read or write on the same
     * socket at the moment? Associate each Connection with a single thread?
     * Need to keep track of Connection-Worker correspondence...
     * Or associate a mutex with each Connection?
     * We can take the mutex and give it up only in the completion handler.
     * When reading we also take the mutex. But this is like synchronous case...
     * Use boost::strand, one per Connection (socket) to ensure sequential
     * handler invokation? We can, but it will be complex...
     * Or just have a dedicated thread with an io_service per Connection? Yes.*/

    try
    {
        std::vector<Channel*> channels;
        for ( ChannelToThreadMap::const_iterator it=mChan2Threads.begin();
              it != mChan2Threads.end(); ++it )
        {
            channels.push_back((it->first).get());
        }

        // Create a Connection
        boost::shared_ptr<Connection> connection(new Connection(channels, this));  // Ref count is 1

        // Asynchronously accept the new connection
        mAcceptor.async_accept(connection->getSocket(),
                               boost::bind(&Server::incomingConnection, this,
                                           connection->shared_from_this(), _1));  // Ref count becomes 2

        // Store connection pointer in the Server to be able to close it later
        mNewConnection = connection.get();
    }  // Temp "connection" is destroyed and ref count becomes 1 (referenced from the io_service)
    catch( std::exception& e )
    {
        LOG( "Error creating client connection acceptor: " << e.what() << std::endl );
        Stop();
        return;
    }

    mIOService.run();  // Blocks until there is work!
}


void Server::Stop()
{
    mStopRequested = true;

    // Cancel listening for new connections.
    mNewConnection->StopRequest();

    // Stop the existing connections
    for( ConnToThreadMap::iterator it = mConn2Threads.begin();
         it != mConn2Threads.end(); /* ++it */)
    {
        it->first->StopRequest();
        it->second->join();
        LOG( "Connection thread " << it->second.get() << ", id "
              << it->second->get_id() << " joined" << std::endl );
        mConn2Threads.erase(it++);
    }

    // Stop the channels
    for( ChannelToThreadMap::iterator it = mChan2Threads.begin();
         it != mChan2Threads.end(); /* ++it */)
    {
        it->first->StopRequest();
        it->second->join();
        LOG( "Channel thread " << it->second.get() << ", id "
              << it->second->get_id() << " joined" << std::endl );
        mChan2Threads.erase(it++);
    }

    // Stop Server's io_service event loop
    mIOService.stop();
}


void Server::incomingConnection( boost::shared_ptr<Connection> connection,
                                 const boost::system::error_code &ec )
{
    LOG("New connection!" << std::endl);

    // XXX Check for errors!

    // Return if we are about to stop
    if ( mStopRequested )
    {
        LOG("Server is stopping. Cancel accepting the new connection..."
            << std::endl);
        return;
    }

    // Connect Channel signals to Connection slots ???

    // Create a worker thread to Start() and run the Connection. 'try' it???
    boost::shared_ptr<boost::thread> thread( new boost::thread(boost::bind(
                                       &Connection::Start, connection.get())) );

    // Wait a little for the thread to start and create new Connection reference
    ::sleep(1);

    mConn2Threads[connection.get()] = thread;

    LOG("Connection " << connection.get() << ": worker thread " << thread.get()
         << ", id " << thread->get_id() << " started"  << std::endl);

    // Add new async wait for new client connections
    try
    {
        std::vector<Channel*> channels;
        for ( ChannelToThreadMap::const_iterator it=mChan2Threads.begin();
              it != mChan2Threads.end(); ++it )
        {
            channels.push_back((it->first).get());
        }

        // Create a new Connection
        boost::shared_ptr<Connection> newConn(new Connection(channels, this));

        // Asynchronously accept the new connection
        mAcceptor.async_accept(newConn->getSocket(),
                               boost::bind(&Server::incomingConnection, this,
                                           newConn->shared_from_this(), _1));

        // Store the connection in the server to be able to close it later
        mNewConnection = newConn.get();
    }
    catch( std::exception& e )
    {
        LOG( "Error creating new client connection acceptor: " << e.what()
              << std::endl );
        Stop();
        return;
    }
}


void Server::closingConnection(Connection* conn)
{
    // Post a request in Server's io_service to wait for and delete the Connection!
    mIOService.dispatch(boost::bind(&Server::deleteConnection, this, conn));  // Or post() ?
}


void Server::deleteConnection(Connection* conn)
{
    if ( mStopRequested )
        return;  // We will do the cleanup in Stop().

    ConnToThreadMap::iterator it = mConn2Threads.find(conn);

    if ( it != mConn2Threads.end() )
    {
        it->second->join();  // Is this safe???
        LOG( "Connection thread " << it->second.get() << ", id "
              << it->second->get_id() << " joined" << std::endl );
        LOG( "Deleting " << conn << " connection from Server's list"
              << std::endl );
        mConn2Threads.erase(it);
    }
    else
    {
        LOG( "Connection " << conn << " not found!" << std::endl );
    }
}


/******************************************************************************/


Channel::Channel(int id, int tickInt) :
    mId(id),
    mTickInterval(tickInt),
    mChanIOService(),
    mDLTimer(mChanIOService, boost::posix_time::seconds(tickInt)),
    mTMutex(),
    mStopRequested(false),
    mSubscribers()
{
}


Channel::~Channel()
{
    LOG( "In ~Channel()" << std::endl );
    if ( not mStopRequested )
        StopRequest();  // Just in case it was not called before we get here.
}


void Channel::Start()
{
    ::srand(mId);
    mDLTimer.async_wait(boost::bind(&Channel::onTick, this));
    mChanIOService.run();
}


void Channel::StopRequest()
{
    boost::lock_guard<boost::mutex> lock(mTMutex);
    mStopRequested = true;
    mDLTimer.cancel();  // XXX 'try' or use an error code!
    mChanIOService.stop();
}


void Channel::onTick()
{
    LOG( "Channel tick. Thread id: " << boost::this_thread::get_id() << std::endl );

    int num = ::rand() % 10;

    /* Serve the subscribers. What if it takes more than mTickInterval? The next
     * tick will be invalid. But this should not happen if we use asynchronous
     * operations - they will be queued.
     * Now: how to send the data?
     * Have a dedicated (UDP?) socket to another client's port for each Connection
     * in the Channel? Requires another socket in the Client too. No.
     * Use Connection's socket? Yes. */
    for( std::set<Connection*>::iterator it = mSubscribers.begin();
         it != mSubscribers.end(); ++it )
    {
#if 1  // Call Connection::sendRandom() directly
        (*it)->sendRandom(mId, num);  // Or call (*(it)->getConIOService()).post(...)? No.
#else  // Send a signal
        boost::signal<void(int, int)> mySignal;
        mySignal.connect(boost::bind(&Connection::sendRandom, (*it), _1, _2));
        mySignal(mId, num);
#endif // 0
    }

    boost::lock_guard<boost::mutex> lock(mTMutex);

    if( not mStopRequested /* and current time < next tick time? */ )
    {
        mDLTimer.expires_at(mDLTimer.expires_at() +
                                     boost::posix_time::seconds(mTickInterval));
        mDLTimer.async_wait(boost::bind(&Channel::onTick, this));
    }
}


void Channel::subscribe( Connection* conn )
{
    boost::lock_guard<boost::mutex> lock(mTMutex);

    mSubscribers.insert(conn);
}


void Channel::unSubscribe( Connection* conn )
{
    boost::lock_guard<boost::mutex> lock(mTMutex);

    mSubscribers.erase(conn);
}


/******************************************************************************/


Connection::Connection( const std::vector<Channel*>& channels, Server* server ) :
    mChannels(channels),
    mServer(server),
    mConIOService(),
    mSocket(mConIOService),
    mStarted(false),
    mStopRequested(false),
    mInBuff(),
    mReceiving(false)
{
    LOG("Connection " << this << " created"<< std::endl);
}


Connection::~Connection()
{
    LOG("~Connection " << this << " is being destroyed"<< std::endl);
    if ( not mStopRequested )
        StopRequest();
}


void Connection::Start()
{
    LOG("In Connection " << this << " Start(), caller thread id: "
         << boost::this_thread::get_id() << std::endl);

    // Add async read operation
    boost::asio::async_read_until(mSocket, mInBuff, '\n',
               boost::bind(&Connection::readyRead, shared_from_this(), _1, _2));  // Contains Connection reference now!

    mStarted = true;

    mConIOService.run();
}


void Connection::StopRequest()
{
    LOG("Connection " << this << " StopRequest(), caller thread id : "
         << boost::this_thread::get_id() << std::endl);

    if ( mStarted and (not mConIOService.stopped()) )
    {
        // Post a request in Connection's io_service to stop the Connection!
        mConIOService.dispatch(boost::bind(&Connection::doStop,  shared_from_this()));  // Or post() ?
    }
    else
    {
        doStop();
    }
}


void Connection::doStop()
{
    LOG("Connection " << this << " doStop(), caller thread id : "
         << boost::this_thread::get_id() << std::endl);

    if ( mStopRequested )
        return;

    mStopRequested = true;

    if ( mReceiving )
    {
        // Unsubscribe from all channels
        for ( std::vector<Channel*>::const_iterator it=mChannels.begin();
              it != mChannels.end(); ++it )
        {
            (*it)->unSubscribe(this);
        }
    }

    // Cancel pending async operations? Does it remve them???
    mSocket.cancel();

    // Close the socket (if it is open?)
    mSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    mSocket.close();

    // Cancel and delete pending async operations somehow???

//  mStarted = false;  // Don't need this.

    // Inform the Server
    mServer->closingConnection(this);

    // Stop the io_service (if it is started?). Do we need this???
    // It will not allow async op flush and references to Connection may remain!!!
//  mConIOService.stop();
}


void Connection::readyRead(const boost::system::error_code & err, size_t readBytes)
{
    LOG("In Connection " << this << " readyRead()" << std::endl);

    // Check for administrative shutdown
    if ( mStopRequested or (boost::asio::error::operation_aborted == err) )
    {
        LOG("Connection stopped!" << std::endl);
        return;
    }

    // Check for disconnect (or any other error???)
    if ( (boost::asio::error::eof == err) || (boost::asio::error::connection_reset == err) )
    {
        LOG("Connection closed! Cleaning up..." << std::endl);
        doStop();
        return;
    }

    std::istream in(&mInBuff);
    std::string msg;
    std::getline(in, msg);
    LOG("'" << msg << "'" << std::endl);

    if ( strncasecmp("start", msg.c_str(), 5) == 0 )
    {
        if ( not mReceiving )
        {
            for ( std::vector<Channel*>::const_iterator it=mChannels.begin();
                  it != mChannels.end(); ++it )
            {
                (*it)->subscribe(this);
            }
            mReceiving = true;
        }
    }
    else if ( strncasecmp("stop", msg.c_str(), 4) == 0 )
    {
        if ( mReceiving )
        {
            for ( std::vector<Channel*>::const_iterator it=mChannels.begin();
                  it != mChannels.end(); ++it )
            {
                (*it)->unSubscribe(this);
            }
            mReceiving = false;
        }
    }
    else if ( strncasecmp("quit", msg.c_str(), 4) == 0 )
    {
        doStop();
        return;
    }

    // Add new async read operation
    if ( not mStopRequested )
    {
        boost::asio::async_read_until(mSocket, mInBuff, '\n',
               boost::bind(&Connection::readyRead, shared_from_this(), _1, _2));
    }
}


void Connection::sendRandom( int id, int num )
{
    LOG("In Connection::sendRandom(), thread id: " << boost::this_thread::get_id()
         << std::endl);

    /* Call async_write() on Connection's socket from another thread? No.
     * Use signal/connect and post() to invoke a method (that will call wite)? */
    mConIOService.post(boost::bind(&Connection::doSendRandom,
                                   shared_from_this(), id, num));  // Or dispatch() ?
}


void Connection::doSendRandom( int id, int num )
{
    LOG("In Connection::doSendRandom(), thread id: " << boost::this_thread::get_id()
         << std::endl);

    if ( not mStopRequested )  // Do we need this???
    {
        std::stringstream buf;  // Or send as numbers with htonl() ???
        buf << " " << id << ":" << num; // << std::endl;

        /* We can use socket.async_send() or async_write(socket, ...).
         * Do we need to protect adding async operations with a mutex? Probably not,
         * if we used post().
         * Do we need an output buffer or a queue, or we can count on Boost to copy
         * and queue the data to be sent???
         * Or we can send the data synchronously using socket.send() or write().
         * In this case we don't need a queue. Yes. */
        mSocket.send(boost::asio::buffer(buf.str()));
    }
}
