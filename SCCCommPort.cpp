#include "SCCCommPort.h"

#include <string>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <cmath>
#include <cstring>
#include "SCCRealTime.h"
#include "../main_control/device.h"


using namespace std;

//#define PRINT_DBG()           {std::cout << "Line:\t" << __LINE__ << "\t" << SCCRealTime::getTimeStamp() << std::endl;}

#ifdef WINDOW_OS
unordered_map<int,DWORD> stBaudRateMap =
{
        {110,CBR_110},
        {300,CBR_300},
        {600,CBR_600},
        {1200,CBR_1200},
        {2400,CBR_2400},
        {4800,CBR_4800},
        {9600,CBR_9600},
        {19200,CBR_19200},
        {38400,CBR_38400},
        {57600,CBR_57600},
        {115200,CBR_115200},
        {128000,CBR_128000},
        {256000,CBR_256000}
};
#endif // WINDOW_OS

SCCCommPort::SCCCommPort()
{
    //ctor
    m_bOpened = false;
    m_bReceived = false;
    m_bSending = false;
    m_bSent = false;

    m_threadRun = NULL;

    m_iCommPort = 1;
    m_iDataLength = 8;
    m_iBaudRate = 9600;
    m_iStopBits = 1;
    m_iParity = 0;

    m_iRxByteCount = 0;
    m_iTxByteCount = 0;

    m_iRxTimeOut    = 0;
    m_iTxTimeOut    = 0;

    m_iBufferInPtr  = 0;
}

SCCCommPort::~SCCCommPort()
{
    //dtor
    closePort();
    for (auto pThread : m_threadList)
    {
        //pThread->join();
        delete pThread;
    }
}

bool SCCCommPort::sendData(char* msg, size_t len)
{
    return writeMsg(msg, len);
}

bool SCCCommPort::sendData(std::string msg)
{
    return writeMsg(msg);
}

bool SCCCommPort::getData(char* buffer, int& len)
{
    *buffer = '\0';
    len = 0;
    if (m_bOpened == false || m_bReceived == false)
        return false;

    //while (m_chBufferIn.size())
    {
        std::lock_guard<std::mutex> lockbuf(m_mutexBuffer);
        if (m_iBufferInPtr)
        {
            memcpy(buffer, m_chBufferIn, m_iBufferInPtr);
            len = m_iBufferInPtr;
            m_iBufferInPtr = 0;
        }
        //*buffer++ = m_chBufferIn.front();
        //m_chBufferIn.pop();
        //++len;
    }
    //*buffer = '\0';
    m_bReceived = false;

    return true;
}

std::string SCCCommPort::printCounter()
{
    std::stringstream ss;

    ss << "Rx:\t" << m_iRxByteCount
        << "\tTx:\t" << m_iTxByteCount
        << "\tLoop:\t" << m_loopCounter
        << "\tLoopRx:\t" << m_iLoopRx;

    return std::string(ss.str());
}

void SCCCommPort::addRxBuffer(int n)
{
    addBuffer(n, m_iRxByteCount);
}

void SCCCommPort::addTxBuffer(int n)
{
    addBuffer(n, m_iTxByteCount);
}

void SCCCommPort::addBuffer(int n, int& bufferCounter)
{
    bufferCounter += n;
    if (bufferCounter >= MAX_BUFFER_THRESHOLD)
        flushBuffers();
}

void SCCCommPort::sleepDuringTxRx(int byteSize)
{
    if (byteSize <= 0)
        return;
    int iTimeOut = std::ceil(byteSize*10000.0/m_iBaudRate);
    if (iTimeOut <= 0)
        return;
    std::this_thread::sleep_for(std::chrono::milliseconds(iTimeOut));
}

void SCCCommPort::killThread(std::thread* pThread)
{
    //PRINT_DBG();
    //delete pThread;
    m_threadList.push_back(pThread);
    /*std::thread* pKillThread ;
    if (pThread)
    {
        pThread->join();
        delete pThread;
    }*/
}

bool SCCCommPort::searchNextPort()
{
    if (!isOpened())
    {
        while(!m_comPortQueue.empty())
        {
            int nPort = m_comPortQueue.front();
            closePort();
            bool bOpened = openPort(nPort, m_iBaudRate);
            m_comPortQueue.pop();
            if (bOpened)
                return true;
        }
    }
    else
    {
        /* Launching another instace */
        //std::string ss;
        /*ss << argv[0];
        ss << " ";*/
        if (m_comPortQueue.empty())
            return false;
        std::string strArgs;
        while (!m_comPortQueue.empty())
        {
            int nPort = m_comPortQueue.front();
            m_comPortQueue.pop();
            if (strArgs == "")
                strArgs = std::to_string(nPort);
            else
                strArgs += std::string(",") + std::to_string(nPort);
        }
        for (int i = 2; i < m_iArgc; ++i)
            strArgs += std::string(" ") + m_pArgv[i];
        //str += " &";
        Device::launchService(m_pArgv[0], strArgs);
     }
    return false;
}

#ifdef WINDOW_OS

void SCCCommPort::stopSearchPort()
{
    while (!m_comPortQueue.empty())
        m_comPortQueue.pop();
    ioctl(m_iUSBPort, TIOCEXCL);
}

bool SCCCommPort::openPort(const int iPort, const int baudRate)
{
    if (m_bOpened == true)
        return true;

    m_iCommPort = iPort;
    m_iBaudRate = baudRate;
    std::string strPort("\\\\.\\COM");
    std::stringstream ss;
    ss << iPort;
    strPort += ss.str();

    DCB dcb;

    m_hPort = CreateFile(
        strPort.c_str(),
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0 /*FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED*/,
        NULL
     );

    SetCommMask (m_hPort, EV_RXCHAR | EV_TXEMPTY | EV_ERR); //receive character event

    if (!GetCommState(m_hPort,&dcb))
        return false;

    dcb.BaudRate = stBaudRateMap[baudRate]; //9600 Baud
    dcb.ByteSize = 8; //8 data bits
    dcb.Parity = NOPARITY; //no parity
    dcb.StopBits = ONESTOPBIT; //1 stop

    if (!SetCommState(m_hPort, &dcb))
        return false;

    //PurgeComm( m_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = MAXDWORD;
    timeout.ReadTotalTimeoutConstant = 0;
    timeout.ReadTotalTimeoutMultiplier = 0;
    timeout.WriteTotalTimeoutConstant = 0;
    timeout.WriteTotalTimeoutMultiplier = 0;

    bool bSetTimeOut = SetCommTimeouts(m_hPort, &timeout);

    if (bSetTimeOut)
        cout << "Time Out set" << std::endl;

    m_hThreadTerm = CreateEvent(0,0,0,0);

    m_bOpened = true;

    cout << "Comm Port " << iPort << " opened." << std::endl;

    //SetCommMask (m_hPort, EV_RXCHAR | EV_ERR); //receive character event

    if (m_threadRun == NULL)
        m_threadRun = new std::thread(&SCCCommPort::main_loop, this);

    return true;
}

void SCCCommPort::main_loop()
{
    if (m_bOpened == true)
    {
       	DWORD dwEventMask = 0;

        OVERLAPPED ov;
        memset(&ov,0,sizeof(ov));
        ov.hEvent = CreateEvent( NULL,TRUE,FALSE,NULL);

        /*HANDLE arHandles[2];
        arHandles[0] = m_hThreadTerm;
       	DWORD dwWait;*/

        SetCommMask (m_hPort, EV_RXCHAR | EV_TXEMPTY | EV_ERR); //receive character event
        //return;
dwEventMask=EV_RXCHAR;
bool ret = true;
        while (m_bOpened == true)
        {
            //bool ret = WaitCommEvent (m_hPort, &dwEventMask, &ov);
            ++m_loopCounter;
            if ((dwEventMask & EV_RXCHAR) == EV_RXCHAR)
            {
                m_bRxEvent = false;
                //std::string msg = readMsg();
                m_Buffer += readMsg();
            }
            else
            {

            }

            if (m_bRxEvent == false)
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            if (ret == false)
                return;

            //ResetEvent ( ov.hEvent );
        } //while
        CloseHandle( ov.hEvent ) ;
        //std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void SCCCommPort::closePort()
{
    if (m_bOpened == true)
    {
        m_bOpened = false;
        if (m_threadRun != NULL)
        {
            m_threadRun->join();
            delete m_threadRun;
        }
        CloseHandle(m_hPort); //close the handle
    }
}

bool SCCCommPort::sendByte(char ch)
{
    if (m_bOpened == false)
        return false;

    DWORD bytesWritten;
    //char ch = m_chBufferOut.front();
    bool retVal = WriteFile(m_hPort, &ch, 1, &bytesWritten, NULL);

    if (retVal == true && bytesWritten == 1)
        return true;
    else
        return false;
}

bool SCCCommPort::writeMsg(std::string msg)
{
    if (m_bOpened == false)
        return false;

    DWORD bytesWritten;
    //char ch = m_chBufferOut.front();
    bool retVal;
    int count = 0;
    //CancelIo(m_hPort);
    do
    {
        ++count;
        retVal = ::WriteFile(m_hPort, msg.c_str(), msg.length(), &bytesWritten, NULL);

        if (retVal == true && bytesWritten > 0)
        {
            msg = msg.substr(bytesWritten);
            count = 0;
        }
        else
            return false;
    }
    while (retVal == true && msg.length() > 0 && count < 5);

    return retVal;
}

bool SCCCommPort::writeMsg(char* msg, size_t len)
{
    if (m_bOpened == false)
        return false;

    DWORD bytesWritten;
    //char ch = m_chBufferOut.front();
    bool retVal;
    int count = 0;
    do
    {
        ++count;
        retVal = WriteFile(m_hPort, msg, 1, &bytesWritten, NULL);

        if (retVal == true && bytesWritten > 0)
        {
            //msg = msg.substr(bytesWritten);
            msg += bytesWritten;
            len -= bytesWritten;
            count = 0;
        }
        else
            return false;
    }
    while (retVal == true && len > 0 && count < 5);

    return retVal;
}

std::string SCCCommPort::readMsg()
{
    if (m_bOpened == false)
        return '\0';

	BOOL       fReadStat = FALSE;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	DWORD      dwError;
	char       szError[ 10 ] ;

	// only try to read number of bytes in queue
	ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
	dwLength = ComStat.cbInQue;

	std::string strMsg;

	if (dwLength > 0)
	{
        OVERLAPPED ovRead;
        memset(&ovRead,0,sizeof(ovRead));
        ovRead.hEvent = CreateEvent( NULL,TRUE,FALSE,NULL);
        ResetEvent( ovRead.hEvent  );

	    char lpszBlock[dwLength + 1];
	    memset(lpszBlock,0, sizeof(lpszBlock));
		fReadStat = ReadFile( m_hPort, lpszBlock,
		                    dwLength, &dwLength, &ovRead ) ;
		if (!fReadStat)
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				while(!GetOverlappedResult( m_hPort,
					&ovRead, &dwLength, TRUE ))
				{
					dwError = GetLastError();
					if(dwError == ERROR_IO_INCOMPLETE)
						// normal result if not finished
						continue;
					else
					{
						// an error occurred, try to recover
						wsprintf( szError, "<CE-%u>", dwError ) ;
						OutputDebugString(szError);
						//WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
						ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
						if ((dwErrorFlags > 0))
						{
							wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
							OutputDebugString(szError);
							//WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
						}
						break;
					}
				}
			}
			else
			{
			    // some other error occurred
			    dwLength = 0 ;
				ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
				if ((dwErrorFlags > 0) )
				{
					wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
					OutputDebugString(szError);
					//WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
				}
			}
		}
        if (dwLength > 0)
        {
            lpszBlock[dwLength] = '\0';
            strMsg += lpszBlock;
            {
                std::lock_guard<std::mutex> lockbuf(m_mutexBuffer);
            //for (DWORD i = 0; i < dwLength ; ++i)
            //    m_chBufferIn.push(lpszBlock[i]);
                if (m_iBufferInPtr + lpszBlock >= MAX_BUFFER_THRESHOLD)
                    lpszBlock = MAX_BUFFER_THRESHOLD - m_iBufferInPtr;
                memcpy(m_chBufferIn+m_iBufferInPtr, lpszBlock, dwLength);
                m_iBufferInPtr += lpszBlock;
            }
            m_bReceived = true;
        }
		CloseHandle(ovRead.hEvent );
	}

    return strMsg;
}

char SCCCommPort::getByte()
{
    if (m_bOpened == false)
        return '\0';

    BYTE Byte;
    DWORD dwBytesTransferred;
    //DWORD dwCommModemStatus;

    //WaitCommEvent (m_hPort, &dwCommModemStatus, 0); //wait for character

    //if (dwCommModemStatus & EV_RXCHAR)
        bool ret = ReadFile (m_hPort, &Byte, 1, &dwBytesTransferred, 0); //read 1
    //else //if (dwCommModemStatus & EV_ERR)
        //retVal = 0x101;
    //    return '\0';
    //retVal = Byte;
    if (ret == true)
        if (dwBytesTransferred == 1)
        return Byte;
    return '\0';
}

std::string SCCCommPort::getData()
{
    if (m_bOpened == false)
        return '\0';

    m_bReceived = false;

    return m_Buffer;
}

void SCCCommPort::flushBuffers()
{
}

#endif // WINDOW_OS

