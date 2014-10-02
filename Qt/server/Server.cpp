#include <cstdlib>
#include <cstring>
#include <sstream>
#include "Server.hpp"


QMutex logMutex;


Server::Server(int port, int chanNum, int timeInt, QObject *parent) :
    QTcpServer(parent),
    mPort(port),
    mChanNum(chanNum),
    mTimeInt(timeInt)
{
}


Server::~Server()
{
    qDebug() << "In Server::~Server()";

    // Stop() should be called before we get here!

    // No need to manually delete resources, if we called Stop() before.
    // If not - resources will be freed anyway (by the OS).
}


void Server::Stop()
{
    Q_EMIT stopRequest();

    // Wait for channel threads to join
    for( std::map<Channel*, QThread*>::const_iterator it = mChan2Threads.begin();
         it != mChan2Threads.end(); ++it )
    {
        it->second->wait();
    }

    // Connection threads are waited for in clientDisconnected()

    Q_EMIT finished();
}


void Server::Start()
{
    qDebug()<<"Main server thread: " << QThread::currentThreadId();

    try
    {
        // Create server channels
//      int delay = 1000 / mChanNum;
        for( int i=0; i<mChanNum; ++i )
        {
            QThread* chanThread = new QThread(this);
            Channel* channel = new Channel(i+1, mTimeInt, chanThread, NULL);  // Parent must be NULL in order to move it to thread
            channel->moveToThread(chanThread);  // Starts the Channel in the new thread
            mChan2Threads[channel] = chanThread;
 
            // Be able to stop it from the main thread
            connect(this, SIGNAL(stopRequest()), channel, SLOT(stopRequested()));

            // This is to ensure automatic startup and cleanup
            connect(chanThread, SIGNAL(started()), channel, SLOT(Start()));
            connect(channel, SIGNAL(finished()), channel, SLOT(deleteLater()));
            connect(chanThread, SIGNAL(finished()), chanThread, SLOT(deleteLater()));

            chanThread->start();

//          QThread::currentThread()->msleep(delay); // Small shift between channels
        }
        qDebug() << "Created " << mChanNum << " channels with timers.";
    }
    catch(...)
    {
        qDebug() << "Error creating channels. Exiting...";
        Stop();
        return;
    }

    if( !this->listen(QHostAddress::Any, mPort) )
    {
        qDebug() << "Could not start server. Exiting...";
        Stop();
        return;
    }
    else
    {
        qDebug() << "Listening on port " << mPort;
    }
}


void Server::incomingConnection(int socketDescriptor)
{
    qDebug() << "Client " << socketDescriptor << " Connecting";

    QThread* connThread = NULL;  // XXX Do we need per connection thread, or we can manage connections in the main thread? Or use a limited thread pool?
    try
    {
        connThread = new QThread(this);
    }
    catch(...)
    {
        qDebug() << "Unable to create connection thread!";
        return;
    }

    Connection* client = NULL;
    try
    {
        std::vector<Channel*> channels;
        for( std::map<Channel*, QThread*>::const_iterator it = mChan2Threads.begin();
             it != mChan2Threads.end(); ++it )
        {
            channels.push_back(it->first);
        }
        client = new Connection(socketDescriptor, channels, connThread, NULL);  // Parent must be NULL in order to moveToThread!
    }
    catch(...)
    {
        qDebug() << "Unable to create new Connection object!";
        connThread->terminate();
        connThread->wait();
        delete connThread;  // Do we need this?
        return;
    }

    client->moveToThread(connThread);
    mConId2Threads[socketDescriptor] = connThread;

    // Communication with the main thread
    connect(client, SIGNAL(connClosed(int)), this, SLOT(clientDisconnected(int)));  // Use Qt::BlockingQueuedConnection?
    connect(this, SIGNAL(stopRequest()), client, SLOT(stopRequested()));

    // This is to ensure automatic startup and cleanup
    connect(connThread, SIGNAL(started()), client, SLOT(Start()));
    connect(client, SIGNAL(finished()), client, SLOT(deleteLater()));
    connect(connThread, SIGNAL(finished()), connThread, SLOT(deleteLater()));

    // Connect the client to server channels. No - do it only on client's request.
#if 0
    for( std::map<Channel*, QThread*>::const_iterator it = mChan2Threads.begin();
         it != mChan2Threads.end(); ++it )
    {
        connect(it->first, SIGNAL(emitRandom(int, int)), client, SLOT(sendRandom(int, int)));
    }
#endif // 0
    connThread->start();
}


void Server::clientDisconnected(int conId)
{
    qDebug() << "Client disconnected. Removing connection " << conId << " data";
    if ( mConId2Threads.find(conId) != mConId2Threads.end() )
    {
        mConId2Threads[conId]->wait();  // Wait for the thread to join.
        mConId2Threads.erase(conId);
    }
    else
    {
        qDebug() << "Connection " << conId << " not found!";
    }
}


/******************************************************************************/


Channel::Channel(int id, int interval, QThread* thread, QObject *parent) :
    QObject(parent),
    mId(id),
    mInterval(interval),
    mQTimer(NULL),
    mThread(thread)
{
}


Channel::~Channel()
{
    qDebug() << "In Channel " << mId << " ~Channel()";
//  mQTimer->stop();
    // QTimer will be deleted automatically
    mThread->quit();
}


void Channel::Start()
{
    mQTimer = new QTimer(this);
    mQTimer->setInterval(1000*mInterval);
    connect(mQTimer, SIGNAL(timeout()), this, SLOT(onTick()));
    mQTimer->start();
}


void Channel::onTick()
{
//  qDebug() << " Channel " << mId << " onTick() called from " << QThread::currentThreadId();
    int num = ::rand() % 10;
    Q_EMIT emitRandom(mId, num);  // Broadcast to subscribers
}


void Channel::stopRequested()
{
    qDebug() << "Channel " << mId << " stopRequested()";
    mQTimer->stop();
    mQTimer->deleteLater();
    Q_EMIT finished();
}


/******************************************************************************/


Connection::Connection(int sId, const std::vector<Channel*>& channels, 
                       QThread* thread, QObject* parent) :
    QObject(parent),
    mSockId(sId),
    mReceiving(false),
    mSocket(NULL),
    mThread(thread),
    mChannels(channels),
    mBufMark(0)
{
}


Connection::~Connection()
{
//  qDebug() << "In Connection " << mSockId << " ~Connection()";
    // Close the socket? Should be closed already. Will be deleted automatically.
    mThread->quit();
}


void Connection::Start()
{
    mSocket = new QTcpSocket(this);
    if( !mSocket->setSocketDescriptor(mSockId) )
    {
        qDebug() << "ERROR in setSocketDescriptor()";
        Q_EMIT finished();
    }
    else
    {
        connect(mSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));  // XXX Use Qt::DirectConnection ?
        connect(mSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    }
    // Send a list of available channels?
}


void Connection::sendRandom(int chanId, int randNum)
{
/*  qDebug() << " Connection " << mSockId << " sendRandom(" << chanId << ", " << randNum 
             << ") called from " << QThread::currentThreadId(); */

    if ( mReceiving )
    {
        std::stringstream buf;  // Or send as numbers with htonl() ???
        buf << " " << chanId << ":" << randNum; // << std::endl;
        QByteArray Data = QByteArray(buf.str().c_str());
        mSocket->write(Data);
        mSocket->flush();  // Cycle or wait until everything is written? No - the event loop will take care.
    }
}


void Connection::readyRead()
{
    int bytesRead = mSocket->read(mBuffer+mBufMark, BUFSIZE-mBufMark-1);
    if( bytesRead < 0 )
    {
        qDebug() << "Error in socket read()";
        // Close the connection?
        return;
    }

    mBuffer[mBufMark+bytesRead] = '\0';

/*  qDebug() << " Connection " << mSockId << ", data in: " << mBuffer+mBufMark 
             << ", called from " << QThread::currentThreadId(); */

    if( NULL == strchr(mBuffer+mBufMark, '\n') )
    {
        mBufMark += bytesRead;
        if( mBufMark >= (BUFSIZE-1) )
        {
            qDebug() << "Ignoring too long command!";
            mBufMark = 0;
        }
        return;
    }
    else
    {
        char *eol, *last_eol, *buf=mBuffer;

        while ( NULL != (eol = strchr(buf, '\n')) )
        {
            *eol = '\0';

            if( strncasecmp("start", buf, 5) == 0 )
            {
                mReceiving = true;
                qDebug() << "Starting acquisition for client " << mSockId;

                // Connect to Channels
                for( std::vector<Channel*>::const_iterator it = mChannels.begin();
                     it != mChannels.end(); ++it )
                {
                    connect(*it, SIGNAL(emitRandom(int, int)), this, SLOT(sendRandom(int, int)));
                }
            }
            else if( strncasecmp("stop", buf, 4) == 0 )
            {
                mReceiving = false;
                qDebug() << "Stopping acquisition for client " << mSockId;

                // Disconnect from Channels
                for( std::vector<Channel*>::const_iterator it = mChannels.begin();
                     it != mChannels.end(); ++it )
                {
                    disconnect(*it, SIGNAL(emitRandom(int, int)), this, SLOT(sendRandom(int, int)));
                }
            }
            else
            {
                qDebug() << "Unknown command: '" << buf << "'";
            }

            last_eol = eol;
            buf = eol + 1;
        }

        if( last_eol == (mBuffer+mBufMark+bytesRead-1) )  // No data after newline
        {
            mBufMark = 0;
        }
        else
        {
            int rest = (mBuffer+mBufMark+bytesRead-1) - last_eol;
            memmove(mBuffer, last_eol+1, rest);
            mBufMark = rest;
        }
    }
}


void Connection::disconnected()
{
    qDebug() << " Connection " << mSockId << " disconnected";
    Q_EMIT connClosed(mSockId);  // Inform the main thread
    mSocket->deleteLater();
    Q_EMIT finished();
}


void Connection::stopRequested()
{
    qDebug() << "Connection " << mSockId << " stopRequested()";

    if ( mSocket->state() != QAbstractSocket::UnconnectedState )
        mSocket->close();  // Will call back disconnected()
    else
        this->disconnected();
}
