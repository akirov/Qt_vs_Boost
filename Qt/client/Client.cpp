#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <QThread>

#include "Client.hpp"


Client::Client( const std::string& server, int port, QObject *parent ) :
    QObject(parent),
    mServerAddr(server),
    mServerPort(port),
    mConnThread(NULL),
    mConnection(NULL)
{
}


Client::~Client()
{
    qDebug() << "In Client::~Client()";

    // Stop() should be called before we get here!

    // Resources are deleted automatically
}


#define BUFSIZE 256

void Client::Start()
{
//  qDebug()<<"Main client thread: " << QThread::currentThreadId();

    try
    {
        mConnThread = new QThread(this);
    }
    catch(...)
    {
        qDebug() << "Unable to create connection thread!";
        Q_EMIT finished();
        return;
    }

    try
    {
        mConnection = new Connection(mServerAddr, mConnThread, mServerPort, NULL);  // Parent must be NULL in order to moveToThread!
    }
    catch(...)
    {
        qDebug() << "Unable to create Connection object!";
        mConnThread->terminate();
        mConnThread->wait();  // Wait for the thread to join
        delete mConnThread;  // Do we need this?
        Q_EMIT finished();
        return;
    }

    mConnection->moveToThread(mConnThread);

    // Communication with the Connection thread
    connect(this, SIGNAL(sendToServer(QString)), mConnection, SLOT(sendToServer(QString)));
    connect(this, SIGNAL(stopRequest()), mConnection, SLOT(stopRequested()));

    // This is to ensure automatic startup and cleanup
    connect(mConnThread, SIGNAL(started()), mConnection, SLOT(Start()));
    connect(mConnection, SIGNAL(finished()), mConnection, SLOT(deleteLater()));
    connect(mConnThread, SIGNAL(finished()), mConnThread, SLOT(deleteLater()));

    mConnThread->start();

    // Opens a "console"
    char cmdbuf[BUFSIZE];
    while(true)  // XXX Or while the connection is not closed?
    {
        std::cout << "CLIENT (start|stop|quit)> ";
        std::cin.getline (cmdbuf, BUFSIZE-2);  // Blocks (also the event loop)!

        if( std::cin.bad() || std::cin.eof() || strncasecmp("quit", cmdbuf, 4) == 0 )
            break;

        if( std::cin.fail() )
        {
            std::cout << "Line is too long!" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        }
        else
        {
            int len = strlen(cmdbuf);
            if( len )
            {
                if( cmdbuf[len-1] != '\n' )
                {
                    cmdbuf[len] = '\n'; // Append newline. We have space.
                    cmdbuf[len+1] = '\0';
                }
                Q_EMIT sendToServer(cmdbuf);
            }
        }
    }

    Stop();
}


void Client::Stop()
{
    Q_EMIT stopRequest();
    mConnThread->wait();  // Wait for the connection thread to join
    Q_EMIT finished();
}


/******************************************************************************/


Connection::Connection( const std::string& server, QThread* netOpsThread, 
                        int port, QObject *parent ) :
    QObject(parent),
    mServerAddr(server),
    mServerPort(port),
    mConnThread(netOpsThread),
    mSocket(NULL)
{
}


Connection::~Connection()
{
    qDebug() << "In Connection::~Connection()";
    mConnThread->quit();
}


void Connection::Start()
{
    try
    {
        mSocket = new QTcpSocket(this);
    }
    catch(...)
    {
        qDebug() << "Error creating socket";
        Q_EMIT finished();
        return;
    }
//  qDebug() << "Created socket " << mSocket;

    // Connect to socket callbacks
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, 
            SLOT(error(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(mSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
//  connect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));

    qDebug() << "Connecting...";

    mSocket->connectToHost(mServerAddr.c_str(), mServerPort);

    if( !mSocket->waitForConnected(1000) )
    {
        qDebug() << "Error in connection: " << mSocket->errorString();
        this->stopRequested();  // XXX Or use invokeMethod?
    }
}


void Connection::error(QAbstractSocket::SocketError socketError)
{
    qDebug() << "An error occured: " << socketError;
}


void Connection::connected()
{
    qDebug() << "Connected!";
}


void Connection::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " Bytes written.";
}


void Connection::readyRead()
{
    qDebug() << "Received: " << mSocket->readAll();
}


void Connection::sendToServer(QString buf)
{
    QByteArray Data = QByteArray(buf.toStdString().c_str());
    mSocket->write(Data);
    mSocket->flush();  // Cycle or wait until everything is written? No - the event loop will take care.
}


void Connection::disconnected()
{
    qDebug() << "Disconnected!";
    mSocket->deleteLater();
    Q_EMIT finished();
}


void Connection::stopRequested()
{
//  qDebug() << "Connection::stopRequested() in " << QThread::currentThreadId() << " thread";

    if ( mSocket->state() != QAbstractSocket::UnconnectedState )
        mSocket->close();  // Will call back disconnected()
    else
        this->disconnected();  // XXX Or use invokeMethod?
}
