#ifndef SCCCOMMPORT_H
#define SCCCOMMPORT_H

#include "Detect_SO.h"

#ifdef WINDOW_OS
#include <windows.h>
#endif // __MINGW32__

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <list>

//#include "SerialComm.h"

#define MAX_BUFFER_THRESHOLD    4096

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
        void setBaudRate(int baudRate) {m_iBaudRate = baudRate;}

        bool m_bOpened;
        std::string m_Buffer;

        int m_iCommPort;
        int m_iDataLength;
        int m_iBaudRate;
        int m_iStopBits;
        int m_iParity;

        bool isRxEvent() {return m_bReceived;}
        bool isOpened() {return m_bOpened;}

        std::string printCounter();

        void sleepDuringTxRx(int byteSize);
        static void getComPortList(std::queue<int>& list, int nport = -1);
        void getComPortList(std::string nport);
        bool searchNextPort();
        void stopSearchPort();
        void setDeviceName(const std::string& strDeviceName) {m_strMyDeviceName=strDeviceName;}
        int getComPort() {return m_iCommPort;}
        void setDeviceConnected() {m_bDeviceConnected=true;}
        bool isDeviceConnected() {return m_bDeviceConnected;}
        void setArgs(int argc, char** argv) {m_iArgc=argc; m_pArgv=argv;}

    protected:

        std::thread* m_threadRun;
        void main_loop();

        std::string readMsg();
        bool writeMsg(std::string msg);
        bool writeMsg(char* msg, size_t len);

        bool sendByte(char byte);
        char getByte();

        void addRxBuffer(int n);
        void addTxBuffer(int n);
        void addBuffer(int n, int& bufferCounter);
        void flushBuffers();

        void killThread(std::thread* pThread);

    private:

#ifdef WINDOW_OS
        HANDLE m_hPort;

        HANDLE	m_hThreadTerm ;
        HANDLE	m_hThread;
        HANDLE	m_hThreadStarted;
        HANDLE	m_hDataRx;
#endif
        int m_iUSBPort;
        //int m_iComPort;

        //std::queue<char> m_chBufferIn;
        char m_chBufferIn[MAX_BUFFER_THRESHOLD];
        size_t m_iBufferInPtr;
        std::queue<char> m_chBufferOut;

        bool m_bSending;
        bool m_bSent;
        bool m_bReceived;
        bool m_bRxEvent;

        int m_iRxByteCount;
        int m_iTxByteCount;

        int m_loopCounter;
        int m_iLoopRx;

        int m_iRxTimeOut;
        int m_iTxTimeOut;

        std::mutex m_mutexBuffer;

        std::list<std::thread*> m_threadList;
        std::queue<int>         m_comPortQueue;

        std::string     m_strMyDeviceName;

        bool m_bDeviceConnected;

        int     m_iArgc;
        char**  m_pArgv;
};

#endif // SCCCOMMPORT_H
