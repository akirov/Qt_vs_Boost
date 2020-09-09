#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "common.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>

#define DEF_TICK_INT_MS 1000
#define DEF_NUM_STREAMS 2
#define MAX_PENDING_CONNECTIONS 10


namespace server
{

class Server
{
  public:
    Server(unsigned short srvCtrlPort=DEF_SRV_CTRL_PORT, unsigned short clnDataPort=DEF_CLN_DATA_PORT,
           unsigned int tickIntervalMs=DEF_TICK_INT_MS, unsigned int numStreams=DEF_NUM_STREAMS);
    ~Server();
    Server(const Server&) = delete;
    Server& operator =(const Server&) = delete;

    void Start();
    void Stop();   // blocks until server stops

  private:
    void StartControlListener();
    void ControlListener();

    void StartDataStreams(unsigned int numStreams);
    void DataStream(unsigned int tickIntervalMs, unsigned int id);
    void SendStreamData(u_int32_t channel, u_int32_t value) const;  // const?

  private:
    struct ClientCtrlCon
    {
        int                  _clientSocket;
        struct sockaddr_in   _clientAddr;
        bool                 _isReceiving;  // Or a list of subscribed-to stream id-s?
        std::vector<uint8_t> _buffer;  // for received commands (interpreted after endl)

    };

  private:
    uint16_t m_srvCtrlPort;
    uint16_t m_clnDataPort;
    unsigned int m_tickIntervalMs;
    unsigned int m_numStreams;

    std::atomic<bool> m_stopRequested;  // Or we could use a promise and a future and check it in the threads...

    mutable int m_controlSocket;  // TCP
    mutable int m_dataSocket;  // UDP

    std::vector<ClientCtrlCon> m_clients;
    std::vector<std::thread> m_streams;
    std::thread m_ctrlThread;

    mutable std::shared_mutex m_rwMutex;  // requires C++17
};

}

#endif // __SERVER_HPP__
