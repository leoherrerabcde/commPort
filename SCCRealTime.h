#ifndef SCCREALTIME_H
#define SCCREALTIME_H

#include <iostream>

class SCCRealTime
{
    public:
        SCCRealTime();
        virtual ~SCCRealTime();

        static std::string getTimeStamp();

    protected:

    private:
};

#endif // SCCREALTIME_H
