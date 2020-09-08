#include "log.hpp"
#include "Server.hpp"
#include <random>

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

    // Create m_dataSocket

    StartDataStreams(m_numStreams);
    StartControlListener();
}


void Server::Stop()
{
    m_stopRequested = true;

    m_ctrlThread.join();

    for(auto& st: m_streams)
    {
        st.join();
    }
}


void Server::StartControlListener()
{
    m_ctrlThread = std::thread(&Server::ControlListener, this);
}


void Server::ControlListener()
{

    // when adding/removing clients do std::unique_lock lock(m_rwMutex)

}


void Server::StartDataStreams(unsigned int numStreams)
{
    for( unsigned int i=0; i<numStreams; i++ )
    {
        m_streams.emplace_back(std::thread(&Server::DataStream, this, m_tickIntervalMs, i+1));
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
    std::shared_lock lock(m_rwMutex);  // Can be locked by multiple readers (streams)
    for( auto& c: m_clients )
    {
        // Send over m_dataSocket
    }
}
