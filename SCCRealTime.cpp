#include "SCCRealTime.h"

#include <ctime>
#include <sstream>
#include <iomanip>

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

    ss << std::setfill('0') << std::setw(2) << now->tm_hour;
    ss << ':';
    ss << std::setfill('0') << std::setw(2) << now->tm_min;
    ss << ':';
    ss << std::setfill('0') << std::setw(2) << now->tm_sec;

    return std::string(ss.str());
}
