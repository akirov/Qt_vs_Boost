#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>


extern boost::mutex logMutex;
#define LOG( text ) \
    do { \
        boost::lock_guard<boost::mutex> lock(logMutex); \
        std::stringstream sstr; \
        sstr << __FILE__ << ":" << __FUNCTION__ << "(): " << text; \
        std::cerr << sstr.str(); \
    } while( false )




#endif // CLIENT_H
