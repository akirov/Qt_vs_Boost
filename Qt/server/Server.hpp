#ifndef SERVER_H
#define SERVER_H

#include <map>

#include <QObject>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QTimer>

#define  SERVER_PORT    4321  // Default port
#define  MAX_CHANNELS   10    // Max number of channels
#define  DEF_CHANNELS   2     // Two channels by default
#define  TICK_INTERVAL  1     // In seconds
#define  BUFSIZE        256   // Command buffer size (chars)


extern QMutex logMutex;
#define LOG( text ) \
    do { \
        QMutexLocker ml(&logMutex); \
        qDebug() << __FILE__ << ":" << __FUNCTION__ << "(): " << text; \
    } while( false )


class Channel;

class Server : public QTcpServer
{
    Q_OBJECT

  public:
    Server( int port=SERVER_PORT, int chanNum=DEF_CHANNELS, 
            int timeInt=TICK_INTERVAL, QObject* parent=NULL );
    ~Server();

  public Q_SLOTS:        // Commands and callbacks (signal receivers).
    void Start();        // Entry point
    void Stop();         // Stop command
    void clientDisconnected(int clId);  // Called when a client is diconnected

  Q_SIGNALS:             // Event publishers (signal emitters). Q_SIGNALS is just 'protected'
    void finished();     // Inform parent that we are done
    void stopRequest();  // Stop command for the channels and client connections

  protected:
    void incomingConnection( int socketDescriptor );  // Called on accepted connection

  private:
    // Private (and undefined) copy constructor and assignment operator?
    int mPort;
    int mChanNum;
    int mTimeInt;
    std::map<Channel*, QThread*> mChan2Threads;  // Channel to its thread
    std::map<int, QThread*> mConId2Threads;  // Connection Id (socket descriptor) to its thread. Don't need the object
    // A mutex for the common resources (the console)?
};



class Channel : public QObject
{
    Q_OBJECT

  public:
    Channel( int id, int interval, QThread* thread, QObject* parent=NULL );
    ~Channel();
    int getId() const { return mId; }

  public Q_SLOTS:
    void Start();                 // Entry point
    void onTick();                // Called on each timer tick
    void stopRequested();         // Stop command handler
//  void subscribe();             // A client can subscribe to this channel. Don't need this. Just connect to emitRandom.

  Q_SIGNALS:
    void emitRandom( int, int );  // Emit channel ID and a random number
    void finished();              // Signal used to delete the resources

  private:
    int mId;
    int mInterval;
    QTimer* mQTimer;              // A timer
    QThread* mThread;             // The corresponding thread
};



class Connection : public QObject
{
    Q_OBJECT

  public:
    Connection( int sId, const std::vector<Channel*>& channels, QThread* thread,
                QObject* parent=NULL );
    ~Connection();

  public Q_SLOTS:
    void Start();                 // Entry point
    void sendRandom( int, int );  // Invoked by the channels to send data via the socket
    void readyRead();             // Called when there is data from the client
    void disconnected();          // When the socket is disconnected
    void stopRequested();         // Stop command handler

  Q_SIGNALS:
    void connClosed(int);         // Signal for the main thread to remove connection data
    void finished();              // Signal used to delete the resources

  private:
    int mSockId;                  // Socket descriptor. Identifies the connection
    bool mReceiving;              // Whether the client wants to receive the channels
    QTcpSocket *mSocket;          // Connection socket
    QThread* mThread;             // Corresponding thread
    std::vector<Channel*> mChannels;  // A list of available channels. Or only id-s? Then we will need the dispatcher
    char mBuffer[BUFSIZE];        // A buffer for the received data (commands)
    int mBufMark;                 // Buffer fill mark [0-BUFSIZE)
};

#endif // SERVER_H
