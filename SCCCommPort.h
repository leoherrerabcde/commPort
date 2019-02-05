#ifndef SCCCOMMPORT_H
#define SCCCOMMPORT_H

#include <iostream>
#include <windows.h>
#include <queue>
#include <thread>

//#include "SerialComm.h"

class SCCCommPort
{
    public:
        SCCCommPort();
        virtual ~SCCCommPort();

        bool openPort(const int iPort);
        bool sendData(std::string msg);
        std::string getData();
        void closePort();

        bool m_bOpened;
        std::string m_Buffer;

        int m_iCommPort;
        int m_iDataLength;
        int m_iBaudRate;
        int m_iStopBits;
        int m_iParity;

    protected:

        std::thread* m_threadRun;
        void main_loop();

        std::string readMsg();
        bool writeMsg(std::string msg);

        bool sendByte(char byte);
        char getByte();

    private:

        HANDLE m_hPort;

        HANDLE	m_hThreadTerm ;
        HANDLE	m_hThread;
        HANDLE	m_hThreadStarted;
        HANDLE	m_hDataRx;

        std::queue<char> m_chBufferIn;
        std::queue<char> m_chBufferOut;

        bool m_bSending;
        bool m_bSent;
        bool m_bReceived;
};

#endif // SCCCOMMPORT_H
