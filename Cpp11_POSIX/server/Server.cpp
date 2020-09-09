#include "log.hpp"
#include "Server.hpp"
#include <random>
#include <cerrno>
#include <cstring>
#include <unistd.h>

using namespace server;


Server::Server(unsigned short srvCtrlPort, unsigned short clnDataPort,
               unsigned int tickIntervalMs, unsigned int numStreams) :
    m_srvCtrlPort(srvCtrlPort),
    m_clnDataPort(clnDataPort),
    m_tickIntervalMs(tickIntervalMs),
    m_numStreams(numStreams),
    m_stopRequested(false),
    m_controlSocket(-1),
    m_dataSocket(-1)
{

}


Server::~Server()
{
    if( ! m_stopRequested ) Stop();
}


void Server::Start()
{
    m_stopRequested = false;

    StartDataStreams(m_numStreams);

    StartControlListener();
}


void Server::Stop()
{
    m_stopRequested = true;

    m_ctrlThread.join();
    if( -1 == m_controlSocket ) close(m_controlSocket);

    for(auto& st: m_streams)
    {
        st.join();
    }
    if( -1 == m_dataSocket ) close(m_dataSocket);
}


void Server::StartControlListener()
{
    if( -1 == m_controlSocket )
    {
        // We can try-catch and set std::exception_ptr to std::current_exception() in catch, if not in main thread
        m_controlSocket = socket(AF_INET , SOCK_STREAM , 0);
        if( -1 == m_controlSocket )
        {
            std::string errorStr = std::strerror(errno);
            LOG("Can't create the control socket, error: " << errorStr);
            throw std::runtime_error(errorStr);
        }

        int yes = 1;
        if( setsockopt(m_controlSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
        {
            close(m_controlSocket);
            m_controlSocket = -1;
            std::string errorStr = std::strerror(errno);
            LOG("Can't set control socket options, error: " << errorStr);
            throw std::runtime_error(errorStr);
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(m_srvCtrlPort);
        memset(&(serverAddr.sin_zero), '\0', 8);
        if( bind(m_controlSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1 )
        {
            close(m_controlSocket);
            m_controlSocket = -1;
            std::string errorStr = std::strerror(errno);
            LOG("Can't bind control socket, error: " << errorStr);
            throw std::runtime_error(errorStr);
        }

        if(listen(m_controlSocket, MAX_PENDING_CONNECTIONS) == -1)
        {
            close(m_controlSocket);
            m_controlSocket = -1;
            std::string errorStr = std::strerror(errno);
            LOG("Can't listen on control socket, error: " << errorStr);
            throw std::runtime_error(errorStr);
        }
    }

    m_ctrlThread = std::thread(&Server::ControlListener, this);
}


void Server::ControlListener()
{

    // when adding/removing clients do std::unique_lock lock(m_rwMutex)

}


void Server::StartDataStreams(unsigned int numStreams)
{
    // Create m_dataSocket

    for( unsigned int i=0; i<numStreams; i++ )
    {
        m_streams.emplace_back(std::thread(&Server::DataStream, this, m_tickIntervalMs, i+1));
        std::this_thread::sleep_for(std::chrono::milliseconds(m_tickIntervalMs/numStreams));
    }
}


void Server::DataStream(unsigned int tickIntervalMs, unsigned int id)
{
    std::mt19937 gen(id);
    std::uniform_int_distribution<u_int32_t> distrib(1, 100);

    while( !m_stopRequested )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(tickIntervalMs));
        u_int32_t nextValue = distrib(gen);
        LOG("stream id " << id << ", tick " << nextValue << std::endl);
        SendStreamData(id, nextValue);
    }
}


void Server::SendStreamData(u_int32_t channel, u_int32_t value)
{
    /* Sends data in stream thread context. Another option is just to push the data
     * into a ring buffer and wake a dedicated sending thread... */
    std::shared_lock lock(m_rwMutex);  // Can be locked by multiple readers (streams)
    for( auto& c: m_clients )
    {
        if( c._isReceiving )
            ; // Send over m_dataSocket
    }
}
