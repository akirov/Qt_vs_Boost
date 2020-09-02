#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <mutex>
#include <iostream>
#include <sstream>


extern std::mutex logMutex;

#define LOG( text ) \
    do { \
        std::lock_guard<std::mutex> lock(logMutex); \
        std::stringstream sstr; \
        sstr << __FILE__ << ":" << __FUNCTION__ << "(): " << text; \
        std::cerr << sstr.str(); \
    } while( false )

#endif // __LOG_HPP__
