#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>


#define  SERVER_PORT    4321  // Default port
#define  DEF_NUM_CHAN   2     // Two channels by default
#define  MAX_NUM_CHAN   10    // Max number of channels
#define  TICK_INTERVAL  1     // In seconds


extern boost::mutex logMutex;
#define LOG( text ) \
    do { \
        boost::lock_guard<boost::mutex> lock(logMutex); \
        std::stringstream sstr; \
        sstr << __FILE__ << ":" << __FUNCTION__ << "(): " << text; \
        std::cerr << sstr.str(); \
    } while( false )


class Channel;                                  // Producer (publisher)

class Connection;                               // Consumer (subscriber)

class Server : /* private */ boost::noncopyable // Dispatcher
{
  public:
    Server(int port=SERVER_PORT, int numChannels=DEF_NUM_CHAN, int tickInt=TICK_INTERVAL);
    ~Server();

    void Start();
    void Stop();

    void incomingConnection( boost::shared_ptr<Connection> connection,
                             const boost::system::error_code &ec );  // Called on accepted connection
    void closingConnection( Connection* conn );  // Called when a client is diconnected

    typedef std::map< boost::shared_ptr<Channel>, boost::shared_ptr<boost::thread> >
            ChannelToThreadMap;  // Or a vector of pairs?
    typedef std::map< Connection*, boost::shared_ptr<boost::thread> >
            ConnToThreadMap;

  private:
    // Private (and undefined) copy constructor and assignment operator (boost::noncopyable).
    void deleteConnection( Connection* conn );

    int mPort;
    int mNumChannels;
    int mTickInterval;
    boost::asio::io_service mIOService;
    boost::asio::ip::tcp::endpoint mEndPoint;
    boost::asio::ip::tcp::acceptor mAcceptor;
    bool mStopRequested;
    Connection* mNewConnection;
    ChannelToThreadMap mChan2Threads;
    ConnToThreadMap mConn2Threads;
};


class Channel  // Put it inside the Server class?
{
  public:
    Channel(int id, int tickInt=TICK_INTERVAL);
    ~Channel();

    void Start();
    void StopRequest();

    void onTick();
    void subscribe( Connection* );
    void unSubscribe( Connection* );

  private:
    int mId;
    int mTickInterval;
    boost::asio::io_service mChanIOService;
    boost::asio::deadline_timer mDLTimer;
    boost::mutex mTMutex;
    bool mStopRequested;
    std::set<Connection*> mSubscribers;  // Or shared_ptr, to avoid sending to a closed Connection ???
};


class Connection : public boost::enable_shared_from_this<Connection>
{
  public:
    Connection( const std::vector<Channel*>& channels, Server* server );
    ~Connection();

    void Start();
    void StopRequest();

    boost::asio::ip::tcp::socket& getSocket() { return mSocket; }

    void sendRandom( int, int );  // Invoked by the channels to send data via the socket (Subscriber's update function)
    void readyRead( const boost::system::error_code&, size_t );  // Called when there is data from the client

  private:
    void doStop();
    void doSendRandom( int, int );

    std::vector<Channel*> mChannels;  // Or shared_ptr(weak_ptr)?
    Server* mServer;
    boost::asio::io_service mConIOService;
    boost::asio::ip::tcp::socket mSocket;
    bool mStarted;
    bool mStopRequested;
    boost::asio::streambuf mInBuff;
    bool mReceiving;  // Or a flag per channel?
};

#endif // SERVER_H
