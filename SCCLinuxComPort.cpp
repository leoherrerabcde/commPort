#include "SCCCommPort.h"

#ifdef LINUX_SO

#include <termios.h>
#include <unordered_map>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

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

bool SCCCommPort::openPort(const int iPort, const int baudRate)
{
    if (m_bOpened == true)
        return true;

    m_iCommPort = iPort;
    std::string strPort("/dev/ttyUSB");
    std::stringstream ss;
    ss << iPort;
    strPort += ss.str();

    m_iUSBPort = open( strPort.c_str(), O_RDWR| O_NOCTTY );

    if (m_iUSBPort < 0)
    {
       std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
       return false;
    }
    m_bOpened = true;

    struct termios tty;
    struct termios tty_old;
    memset (&tty, 0, sizeof tty);

    /* Error Handling */
    if ( tcgetattr ( m_iUSBPort, &tty ) != 0 ) {
       std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }

    /* Save old tty parameters */
    tty_old = tty;

    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)stLinuxBaudRateMap[baudRate]);
    cfsetispeed (&tty, (speed_t)stLinuxBaudRateMap[baudRate]);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
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
    }
    std::cout << "Comm Port " << iPort << " opened." << std::endl;

    //SetCommMask (m_hPort, EV_RXCHAR | EV_ERR); //receive character event

    if (m_threadRun == NULL)
        m_threadRun = new std::thread(&SCCCommPort::main_loop, this);

    return true;
}

void SCCCommPort::main_loop()
{
    if (m_bOpened == true)
    {
        while (m_bOpened == true)
        {
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
        return '\0';

    int n = 0,
        spot = 0;
    char buf = '\0';

    /* Whole response*/
    char response[1024];
    memset(response, '\0', sizeof response);
    std::string strMsg;

    do {
        n = read( m_iUSBPort, &buf, 1 );
        if (n >0)
        {
            response[spot] = buf;
            m_chBufferIn.push(buf);
            strMsg += buf;
            spot += n;
            m_bRxEvent = true;
            m_bReceived = true;
        }
    } while(n > 0);

    return strMsg;
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
        close(m_iUSBPort);
    }
}


#endif // LINUX_SO
