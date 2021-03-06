#include "SCCCommPort.h"
#include "../main_control/SCCFileManager.h"
#include "SCCRealTime.h"

//#include <termios.h>
#include <sys/ioctl.h>
#include <string>


#ifdef LINUX_SO

#include <termios.h>
#include <unordered_map>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

//#define PRINT_DBG()           {std::cout << "Line:\t" << __LINE__ << "\t" << SCCRealTime::getTimeStamp() << std::endl;}

static std::unordered_map<int,speed_t> stLinuxBaudRateMap =
{
        {110,B110},
        {300,B300},
        {600,B600},
        {1200,B1200},
        {2400,B2400},
        {4800,B4800},
        {9600,B9600},
        {19200,B19200},
        {38400,B38400},
        {57600,B57600},
        {115200,B115200}
        /*{128000,B128000},
        {256000,B256000}*/
};

void SCCCommPort::getComPortList(std::string nport)
{
	size_t pos_ini = 0, pos;

	while (pos_ini != std::string::npos && pos_ini < nport.size())
	{
		pos = nport.find(',', pos_ini);
		int i;
		if (pos != std::string::npos)
		{
            std::string com = nport.substr(pos_ini, pos - pos_ini);
            i = std::stoi(com.c_str());
			//m_comPortQueue.push(i);
			++pos;
		}
		else
		{
            std::string com = nport.substr(pos_ini);
            i = std::stoi(com.c_str());
			//m_comPortQueue.push(i);
			//list.push_back(nport.substr(pos_ini));
        }
        std::string strPort("/dev/ttyUSB");
        strPort += std::to_string(i);
        if (SCCFileManager::isFileExist(strPort))
            m_comPortQueue.push(i);
		pos_ini = pos;
	}

    //getComPortList(m_comPortQueue, nport);
}

void SCCCommPort::stopSearchPort()
{
    while (!m_comPortQueue.empty())
        m_comPortQueue.pop();
    ioctl(m_iUSBPort, TIOCEXCL);
}

void SCCCommPort::getComPortList(std::queue<int>& list, int nport)
{
    list.push(nport);

    for (int i=0; ;++i)
    {
        if (i == nport)
            continue;
        std::string strPort("/dev/ttyUSB");
        strPort += std::to_string(i);
        if (SCCFileManager::isFileExist(strPort))
            list.push(i);
        else
            break;
    }
}

bool SCCCommPort::openPort(const int iPort, const int baudRate)
{
    if (m_bOpened == true)
        return true;

    m_iCommPort = iPort;
    m_iBaudRate = baudRate;
    std::string strPort("/dev/ttyUSB");
    std::stringstream ss;
    ss << iPort;
    strPort += ss.str();

    std::cout << m_strMyDeviceName << ": Opening " << strPort << " ..." << std::endl;

    m_iUSBPort = open( strPort.c_str(), O_RDWR| O_NOCTTY );

    if (m_iUSBPort < 0)
    {
       std::cout << m_strMyDeviceName << ": Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
       return false;
    }
    //ioctl(m_iUSBPort, TIOCEXCL);
    m_bOpened = true;

    /*fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    FD_SET(m_iUSBPort, &read_fds);
    struct timeval timeout;
    int rv;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    rv = select(m_iUSBPort + 1, &read_fds, &write_fds, &except_fds, &timeout);
    if (rv > 0)
        std::cout << m_strMyDeviceName << ": Read Time out set" << std::endl;
    else
        std::cout << m_strMyDeviceName << ": Problem happened setting read timeout" << std::endl;*/

    struct termios tty;
    struct termios tty_old;
    memset (&tty, 0, sizeof tty);

    /* Error Handling */
    if ( tcgetattr ( m_iUSBPort, &tty ) != 0 ) {
       std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
       closePort();
       return false;
    }

    /* Save old tty parameters */
    tty_old = tty;

    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)stLinuxBaudRateMap[baudRate]);
    cfsetispeed (&tty, (speed_t)stLinuxBaudRateMap[baudRate]);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    //tty.c_cflag     |=  CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);

    /* Flush Port, then applies attributes */
    tcflush( m_iUSBPort, TCIFLUSH );
    if ( tcsetattr ( m_iUSBPort, TCSANOW, &tty ) != 0) {
       std::cout << "Error " << errno << " from tcsetattr" << std::endl;
       closePort();
       return false;
    }
    std::cout << m_strMyDeviceName << ": Comm Port " << iPort << " opened." << std::endl;

    //SetCommMask (m_hPort, EV_RXCHAR | EV_ERR); //receive character event

    if (m_threadRun == NULL)
        m_threadRun = new std::thread(&SCCCommPort::main_loop, this);

    return true;
}

void SCCCommPort::main_loop()
{
    m_loopCounter = 0;
    m_iLoopRx = 0;
    if (m_bOpened == true)
    {
        while (m_bOpened == true)
        {
            ++m_loopCounter;
            m_bRxEvent = false;
            m_Buffer += readMsg();

            if (m_bRxEvent == false)
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
        } //while
    }
}

bool SCCCommPort::writeMsg(std::string msg)
{
    if (m_bOpened == false)
        return false;

    int bytesWritten;
    //char ch = m_chBufferOut.front();
    //bool retVal;
    int count = 0;
    //CancelIo(m_hPort);
    do
    {
        ++count;
        bytesWritten = write(m_iUSBPort, msg.c_str(), msg.length());

        if (bytesWritten > 0)
        {
            msg = msg.substr(bytesWritten);
            count = 0;
            addTxBuffer(bytesWritten);
        }
        else
            return false;
    }
    while (msg.length() > 0 && count < 5);

    return true;
}

bool SCCCommPort::writeMsg(char* msg, size_t len)
{
    if (m_bOpened == false)
        return false;

    int n_written = 0;
    int count = 0;

    do
    {
        n_written = write( m_iUSBPort, msg, len );
        if (n_written > 0)
        {
            len -= n_written;
            msg += n_written;
            addTxBuffer(n_written);
            count = 0;
        }
        else
        {
            ++count;
        }
        //spot += n_written;
    }
    while (len > 0 && count < 5);

    return (len == 0);
}

std::string SCCCommPort::readMsg()
{
    if (m_bOpened == false)
        return "";

    int n = 0;
        //spot = 0;
    char buf = '\0';

    /* Whole response*/
    /*char response[1024];
    memset(response, '\0', sizeof response);*/
    std::string strMsg;

    do {
        ++m_iLoopRx;
        n = read( m_iUSBPort, &buf, 1 );
        if (n >0)
        {
            std::lock_guard<std::mutex> lockbuf(m_mutexBuffer);
            //response[spot] = buf;
            //m_chBufferIn.push(buf);
            //strMsg += buf;
            //spot += n;
            if (m_iBufferInPtr < MAX_BUFFER_THRESHOLD)
                m_chBufferIn[m_iBufferInPtr++] = buf;
            m_bRxEvent = true;
            m_bReceived = true;
            addRxBuffer(n);
        }
    } while(n > 0);

    return strMsg;
}

void SCCCommPort::closePort()
{
    if (m_bOpened == true)
    {
        m_bOpened = false;
        //PRINT_DBG();
        close(m_iUSBPort);
        //ioctl(m_iUSBPort, TIOCNXCL);
        killThread(m_threadRun);
        m_threadRun = NULL;
        /*if (m_threadRun != NULL)
        {
            m_threadRun->join();
            PRINT_DBG();
            delete m_threadRun;
            m_threadRun = NULL;
        }*/
        std::cout << m_strMyDeviceName << ": Com Port " << m_iCommPort << " closed" << std::endl;
    }
}

void SCCCommPort::flushBuffers()
{
return;
    tcflush( m_iUSBPort, TCIFLUSH );
    m_iRxByteCount = 0;
    m_iTxByteCount = 0;
}


#endif // LINUX_SO
