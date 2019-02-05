#include "SCCCommPort.h"

#include <string>
#include <sstream>

using namespace std;

SCCCommPort::SCCCommPort()
{
    //ctor
    m_bOpened = false;
    m_threadRun = NULL;

    m_iCommPort = 1;
    m_iDataLength = 8;
    m_iBaudRate = 9600;
    m_iStopBits = 1;
    m_iParity = 0;
}

SCCCommPort::~SCCCommPort()
{
    //dtor
}

bool SCCCommPort::openPort(const int iPort)
{
    if (m_bOpened == true)
        return true;

    m_iCommPort = iPort;
    std::string strPort("\\\\.\\COM");
    std::stringstream ss;
    ss << iPort;
    strPort += ss.str();

    DCB dcb;

    m_hPort = CreateFile(
        strPort.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
     );

    SetCommMask (m_hPort, EV_RXCHAR | EV_TXEMPTY | EV_ERR); //receive character event

    if (!GetCommState(m_hPort,&dcb))
        return false;

    dcb.BaudRate = CBR_9600; //9600 Baud
    dcb.ByteSize = 8; //8 data bits
    dcb.Parity = NOPARITY; //no parity
    dcb.StopBits = ONESTOPBIT; //1 stop

    if (!SetCommState(m_hPort, &dcb))
        return false;

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

        while (m_bOpened == true)
        {
            bool ret = WaitCommEvent (m_hPort, &dwEventMask, &ov);

            if ((dwEventMask & EV_RXCHAR) == EV_RXCHAR)
            {
                m_Buffer += readMsg();
            }
            else
            {

            }

            if (ret == false)
                return;

            ResetEvent ( ov.hEvent );
        } //while


        std::this_thread::sleep_for(std::chrono::milliseconds(2));
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

bool SCCCommPort::sendData(std::string msg)
{
    return writeMsg(msg);

    /*if (m_bOpened == false)
        return false;

    for (auto ch : msg)
    {
        m_chBufferOut.push(ch);
    }
    m_bSending = true;

    return true;*/
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
    do
    {
        ++count;
        retVal = WriteFile(m_hPort, msg.c_str(), msg.length(), &bytesWritten, NULL);

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

std::string SCCCommPort::readMsg()
{
    if (m_bOpened == false)
        return '\0';

	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	//DWORD      dwError;
	//char       szError[ 10 ] ;

	// only try to read number of bytes in queue
	ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
	dwLength = min( (DWORD) 255, ComStat.cbInQue ) ;

	std::string strMsg;

	if (dwLength > 0)
	{
        OVERLAPPED ovRead;
        memset(&ovRead,0,sizeof(ovRead));
        ovRead.hEvent = CreateEvent( 0,true,0,0);
        ResetEvent( ovRead.hEvent  );

	    char lpszBlock[dwLength + 1];
		fReadStat = ReadFile( m_hPort, lpszBlock,
		                    dwLength, &dwLength, &ovRead ) ;
		if (!fReadStat)
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
			}
			else
			{
			    // some other error occurred
			}
		}
		else
        {
            if (dwLength > 0)
            {
                lpszBlock[dwLength] = '\0';
                strMsg += lpszBlock;
            }
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

    if (m_chBufferIn.empty() == true)
        return "";

    std::string strData;
    do
    {
        strData += m_chBufferIn.front();
        m_chBufferIn.pop();
    }
    while (m_chBufferIn.empty() == false);

    return strData;
}
