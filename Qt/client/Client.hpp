#ifndef CLIENT_H
#define CLIENT_H

#include <string>

#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QThread>
#include <QString>

#define  SERVER_PORT    4321    // Or const int... XXX Put it in common header?


class Connection;

class Client : public QObject
{
    Q_OBJECT

  public:
    Client( const std::string& server, int port=SERVER_PORT, QObject* parent=NULL );
    ~Client();

  public Q_SLOTS:               // Event handlers
    void Start();               // Entry point. Creates a Connection to the server
    void Stop();                // Stop command

  Q_SIGNALS:                    // Event publishers. Q_SIGNALS is just 'protected'
    void sendToServer(QString); // Send commands to the server through the Connection
    void stopRequest();         // Stop the Connection
    void finished();            // Inform parent that we are done

  private:
    // Private (and undefined) copy constructor and assignment operator?
    std::string mServerAddr;
    int mServerPort;
    QThread* mConnThread;       // Separate thread for network operations
    Connection* mConnection;    // Network (socket) operations
    // Add a mutex for the common resources (console)?
};



class Connection : public QObject  // Wraps the socket. Put it inside the client?
{
    Q_OBJECT

  public:
    Connection( const std::string& server, QThread* netOpsThread,
                int port=SERVER_PORT, QObject* parent=NULL );
    ~Connection();

  public Q_SLOTS:                  // Event handlers
    void Start();                  // Entry point
    void stopRequested();          // Handles stop request
    void sendToServer(QString);    // Request from the Client to send data
    void connected();              // Socket hooks follow...
    void disconnected();
    void bytesWritten(qint64);
    void readyRead();
    void error(QAbstractSocket::SocketError);

  Q_SIGNALS:                       // Event publishers. Q_SIGNALS is just 'protected'
    void finished();               // Inform that we are done

  private:
    std::string mServerAddr;
    int mServerPort;
    QThread* mConnThread;
    QTcpSocket *mSocket;
};


#endif // CLIENT_H
