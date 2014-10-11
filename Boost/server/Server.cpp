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
    mConnections()
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

        // Store connection pointer in the Server to be able to stop it later
        mNewConnection = connection.get();
    }  // Temp "connection" is destroyed and ref count becomes 1 (referenced from the io_service)
    catch( std::exception& e )
    {
        LOG( "Error creating client connection acceptor: " << e.what() << std::endl );
        Stop();
        return;
    }

    mIOService.run();  // Blocks!
}


void Server::Stop()
{
    mStopRequested = true;

    // Cancel listening for new connections.
    mNewConnection->StopRequest();

    // Stop the existing connections
    for( std::set< Connection* >::iterator it = mConnections.begin();
         it != mConnections.end(); /* ++it */)
    {
        (*it)->StopRequest();  // Also waits for Connection's thread to join...
        mConnections.erase(it++);
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

    // Stop Server's io_service event lop
    mIOService.stop();
}


void Server::incomingConnection( boost::shared_ptr<Connection> connection,
                                 const boost::system::error_code &ec)
{
    LOG("New connection!" << std::endl);

//  if ( mConnections.size() > 1 ) { Stop(); return; }  // XXX For testing only!

    // XXX Check for errors!

    // Return if we are about to stop
    if ( mStopRequested )
    {
        LOG("Server is stopping. Cancel accepting the new connection..."
            << std::endl);
        return;
    }

    // Connect Channel signals to Connection slots ?

    // Create and start Connection's worker thread
    connection->Start();

    // Wait a little for the thread to start and create new reference
    ::sleep(1);

    mConnections.insert(connection.get());

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

        // Store the connection in the server to be able to stop it later
        mNewConnection = newConn.get();  // connection's ref is decreased!
    }
    catch(std::exception& e)
    {
        LOG( "Error creating new client connection acceptor: " << e.what()
              << std::endl );
        Stop();
        return;
    }
}


void Server::closingConnection(Connection* conn)
{
    // Lock a mutex!

    if ( mStopRequested )
        return;  // We will do the cleanup in Stop().

    /* Post a request in Server's io_service to delete the Connection?
     * No need for this, if we are not waiting for Connection's thread here. */

    std::set< Connection* >::iterator it = mConnections.find(conn);
    if ( it != mConnections.end() )
    {
        LOG( "Deleting " << conn << " connection from Server's list"
              << std::endl );
        mConnections.erase(it);
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
    mDLTimer.cancel();  // XXX try or put an error code!
    mChanIOService.stop();
}


void Channel::onTick()
{
    LOG( "Channel tick in thread id: " << boost::this_thread::get_id() << std::endl );

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
    mStopRequested(false),
    mInBuff(),
    mReceiving(false),
    mCThread()
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
    LOG("In Connection " << this << " Start(), caller thread id : "
         << boost::this_thread::get_id() << std::endl);

    // Create the worker thread
    if ( (not mStopRequested) and (not mCThread) )
    {
        mCThread.reset(new boost::thread(boost::bind(&Connection::doStart, this)));
    }
}


void Connection::doStart()
{
    LOG("In Connection " << this << " doStart(), worker thread : " << mCThread.get()
         << ", id " << mCThread->get_id() << std::endl);

    // Add async read operation. mConIOService holds Connection reference now!
    boost::asio::async_read_until(mSocket, mInBuff, '\n',
               boost::bind(&Connection::readyRead, shared_from_this(), _1, _2));

    mConIOService.run();
}


void Connection::StopRequest( bool waitForThread )
{
    LOG("Connection " << this << " StopRequest(), caller thread id : "
         << boost::this_thread::get_id() << std::endl);

    // Lock a mutex?

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

    // Cancel pending async operations? Does it remove them???
    mSocket.cancel();

    // Close the socket (if it is open?)
    mSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    mSocket.close();

    // Wait a little if we are called from another thread to allow async op flush???
    // Or cancel and delete pending async operations somehow???

    // Inform the Server!
    mServer->closingConnection(this);

    // Stop the io_service (if it is started?). Do we need this???
    // This will not allow async op flush and references to Connection may remain!!!
//  mConIOService.stop();
//  mConIOService.reset();  // Do we need this?

    if ( mCThread and waitForThread )  // and (mCThread->get_id() != boost::this_thread::get_id()) ???
    {
        mCThread->join();
        LOG("Thread " << mCThread.get() << " joined" << std::endl);
        mCThread.reset();  // Do we need this?
    }
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
        LOG("Connection closed!" << std::endl);
        StopRequest(false);
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
    if ( not mStopRequested )
    {
        mConIOService.post(boost::bind(&Connection::doSendRandom,
                                       shared_from_this(), id, num));  // Or dispatch() ?
    }
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
