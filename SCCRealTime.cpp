#include "SCCRealTime.h"

#include <ctime>
#include <sstream>


SCCRealTime::SCCRealTime()
{
    //ctor
}

SCCRealTime::~SCCRealTime()
{
    //dtor
}

std::string SCCRealTime::getTimeStamp()
{
    std::time_t t = std::time(0);   // get time now
    std::tm* now = std::localtime(&t);
    std::stringstream ss;

    ss << (now->tm_hour ) << ':'
         << (now->tm_min) << ':'
         <<  now->tm_sec;
    return std::string(ss.str());
}
