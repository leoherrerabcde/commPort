#ifndef SCCCOMMPORT_H
#define SCCCOMMPORT_H

#include "Detect_SO.h"

#ifdef WINDOW_OS
#include <windows.h>
#endif // __MINGW32__

#include <iostream>
#include <queue>
#include <thread>

//#include "SerialComm.h"

class SCCCommPort
{
    public:
        SCCCommPort();
        virtual ~SCCCommPort();

        bool openPort(const int iPort, const int baudRate);
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

#ifdef WINDOW_OS
        HANDLE m_hPort;

        HANDLE	m_hThreadTerm ;
        HANDLE	m_hThread;
        HANDLE	m_hThreadStarted;
        HANDLE	m_hDataRx;
#endif
        int m_iUSBPort;

        std::queue<char> m_chBufferIn;
        std::queue<char> m_chBufferOut;

        bool m_bSending;
        bool m_bSent;
        bool m_bReceived;
        bool m_bRxEvent;
};

#endif // SCCCOMMPORT_H
