#include "Server.hpp"


int main( int argc, char* argv[] )
{
    LOG( "Main thread : " << boost::this_thread::get_id() << std::endl );

    // Get cmd line arguments

    // Create and Start the server
    Server server;
    server.Start();

    return 0;
}
