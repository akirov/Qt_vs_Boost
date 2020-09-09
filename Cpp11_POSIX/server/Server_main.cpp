#include "log.hpp"
#include "Server.hpp"


int main( int argc, char* argv[] )
{
    LOG("Creating POSIX server..." << std::endl);

    // TODO Get ports from the cmd line

    server::Server myServer{};
    myServer.Start();

    // Wait some time. Or wait for a signal...
    std::this_thread::sleep_for(std::chrono::seconds(10));

    myServer.Stop();

    return 0;
}
