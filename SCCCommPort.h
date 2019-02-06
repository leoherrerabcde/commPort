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
        bool sendData(char* bufferOut, size_t len);
        std::string getData();
        bool getData(char* buffer, int& len);
        void closePort();

        bool m_bOpened;
        std::string m_Buffer;

        int m_iCommPort;
        int m_iDataLength;
        int m_iBaudRate;
        int m_iStopBits;
        int m_iParity;

        bool isRxEvent() {return m_bReceived;}
        bool isOpened() {return m_bOpened;}

    protected:

        std::thread* m_threadRun;
        void main_loop();

        std::string readMsg();
        bool writeMsg(std::string msg);
        bool writeMsg(char* msg, size_t len);

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
