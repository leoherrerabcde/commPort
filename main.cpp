#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <algorithm>

#include "SCCCommPort.h"
#include "SCCWirelessRcvrProtocol.h"
#include "SCCRealTime.h"
#include "SCCArgumentParser.h"
#include "../main_control/CSocket.h"
#include "../main_control/SCCDeviceNames.h"
#include "../main_control/SCCDeviceParams.h"
#include "../main_control/SCCLog.h"


bool    gl_bVerbose(true);
SCCLog  globalLog(std::cout);


using namespace std;

//#define PRINT_DBG()           {std::cout << "Line:\t" << __LINE__ << "\t" << SCCRealTime::getTimeStamp() << std::endl;}

static bool st_bSendMsgView = false;
static bool st_bRcvMsgView  = true;

std::string globalMyDeviceName(MY_DEVICE_NAME);

SCCCommPort commPort;
CSocket sckComPort;

bool bConnected     = false;

std::string firstMessage()
{
    std::stringstream ss;

    ss << FRAME_START_MARK;
    ss << DEVICE_NAME << ":" << DEVICE_RFID_BOQUILLA << ",";
    ss << SERVICE_PID << ":" << getpid() << ",";
    ss << PARAM_COM_PORT << ":" << commPort.getComPort();
    ss << FRAME_STOP_MARK;

    return std::string(ss.str());
}

void printMsg(const std::string& msg = "")
{
    if (sckComPort.isConnected())
    {
        if (bConnected == false)
        {
            bConnected = true;
            sckComPort.sendData(firstMessage());
            std::cout << MY_DEVICE_NAME << ": Socket connected." << std::endl;
        }
        if (msg != "")
            sckComPort.sendData(msg);
    }
    else
    {
        std::cout << SCCRealTime::getTimeStamp() << ',' << msg << std::endl;
    }
}

int main(int argc, char* argv[])
{
    //int nCount          = 0;
    std::string         nPort;
    int baudRate        = 9600;
    float fTimeFactor   = 1.0;
    int remotePort      = 0;

    //bool bConnecting    = false;
    //int  iSckCounter    = 0;

    if (argc > 2)
    {
        nPort = argv[1];
        baudRate = std::stoi(argv[2]);
        if (argc > 3)
            remotePort = std::stoi(argv[3]);
        if (argc > 4)
        {
            std::string strArg(argv[4]);
            if (std::all_of(strArg.begin(), strArg.end(), ::isdigit))
                fTimeFactor = std::stof(argv[4]);
            else
                if (strArg == "ViewSend")
                    st_bSendMsgView = true;
        }
    }

    if (remotePort)
    {
        sckComPort.setSocketName(MY_DEVICE_NAME);
        sckComPort.connect("127.0.0.1", remotePort);
        //bConnecting = true;
        bConnected  = false;
        //iSckCounter = 0;
    }

    commPort.setDeviceName(MY_DEVICE_NAME);
    commPort.setArgs(argc, &argv[0]);
    SCCWirelessRcvrProtocol rcvrProtocol;
    SCCRealTime clock;

    //std::queue<int> comPortQueue;
    commPort.setBaudRate(baudRate);
    commPort.getComPortList(/*comPortQueue, */nPort);

    //commPort.openPort(nPort, baudRate);

    char bufferOut[255];
    char bufferIn[250];
    char chLen;
    char len;
    int iAddr = 1;
    std::string msg;

    msg = rcvrProtocol.getStrCmdStatusCheck(iAddr, bufferOut, chLen);

    msg = rcvrProtocol.convChar2Hex(bufferOut, chLen);

    if (st_bSendMsgView)
        std::cout << "Message: " << msg << " sent." << std::endl;
    //commPort.sendData(bufferOut, len);

    if (st_bSendMsgView)
        cout << "Waiting for response" << std::endl;
    msg = "";

    int iTimeOut;
    bool bNextAddr;
    //char chLen = len;
    //char chLenLast = 0;
    int iNoRxCounter = 0;
    //int iKillSelf = 0;
    //int iWaitForValidRxCounter = 5;
    do
    {
        /*if (!bConnected && sckComPort.isConnected())
            printMsg();*/
        bNextAddr = true;
        iTimeOut = 500;
        //PRINT_DBG();
        if (iNoRxCounter >= 5)
        {
            iNoRxCounter = 0;
            rcvrProtocol.getStrCmdStatusCheck(iAddr, bufferOut, chLen);
        }
        if (chLen > 0)
        {
            if (!commPort.isDeviceConnected() && !commPort.searchNextPort())
                break;
                /*while(!comPortQueue.empty())
                {
                    int nPort = comPortQueue.front();
                    commPort.closePort();
                    bool bOpened = commPort.openPort(nPort, baudRate);
                    comPortQueue.pop();
                    if (bOpened)
                        break;
                }*/
            /*if (st_bSendMsgView)
            {
                cout << commPort.printCounter() << std::endl;
                msg = rcvrProtocol.convChar2Hex(bufferOut, chLen);
                cout << SCCRealTime::getTimeStamp() << ',' << "Sending Message: " << msg << std::endl;
            }*/
            commPort.sendData(bufferOut, chLen);
            commPort.sleepDuringTxRx(3*chLen);
            //chLenLast = chLen;
            chLen = 0;
            iTimeOut = 20;
            bNextAddr = false;
        }
        if (commPort.isRxEvent() == true)
        {
            iNoRxCounter = 0;
            bNextAddr =false;
            int iLen;
            bool ret = commPort.getData(bufferIn, iLen);
            if (ret == true)
            {
                len = (char)iLen;
                /*if (st_bRcvMsgView)
                {
                    msg = rcvrProtocol.convChar2Hex(bufferIn, len);
                    std::cout << " Buffer In(Hex): [" << msg << "]" << std::endl;
                }*/
                std::string strCmd;
                char resp[256];
                int addr = 0;
                char respLen = 0;
                bool bIsValidResponse = rcvrProtocol.getWGTResponse(bufferIn, len, strCmd, addr, resp, respLen);
                bool bNextAction = false;
                if (bIsValidResponse == true)
                {
                    //iWaitForValidRxCounter = 0;
                    /*while(!comPortQueue.empty())
                    {
                        comPortQueue.pop();
                    }*/
                    commPort.setDeviceConnected();
                    commPort.stopSearchPort();
                    if (st_bRcvMsgView)
                    {
                        //cout << ++nCount << " " << commPort.printCounter() << clock.getTimeStamp() << " Valid WGT Response" << std::endl;
                        /*if (strCmd == CMD_CHECKSTATUS)
                            cout << ++nCount << " WGT Status: " << rcvrProtocol.getStrStatus(resp[0]) << endl;*/
                    }
                    bNextAction = rcvrProtocol.nextAction(iAddr, bufferOut, chLen, iTimeOut);
                    if (bNextAction == true)
                    {
                        if (st_bRcvMsgView)
                        {
                            //std::stringstream ss;
                            //ss << ++nCount << " " << commPort.printCounter() << rcvrProtocol.printStatus(iAddr) << std::endl;
                            printMsg(rcvrProtocol.printStatus(iAddr));
                        }
                        //iTimeOut = 0;
                    }
                }
            }
        }
        if (iTimeOut > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(iTimeOut*fTimeFactor)));
        if (bNextAddr == true)
        {
            //++iAddr;
        }
        if (/*bConnecting ==true || */bConnected == true)
        {
            if (sckComPort.getSocketState() == sckError)
            {
                if (remotePort)
                {
                    sckComPort.connect("127.0.0.1", remotePort);
                    //bConnecting = true;
                    bConnected  = false;
                    //iSckCounter = 0;
                }
            }
            /*if (sckComPort.isConnected())
            {
                //bConnecting = false;
                bConnected  = true;
            }*/
        }
        ++iNoRxCounter;
        if (bConnected == true && !sckComPort.isConnected())
            break;
    }
    while (commPort.isOpened());
    std::cout << MY_DEVICE_NAME << ": Exiting the application normally" << std::endl;
    sckComPort.disconnect();
    commPort.closePort();
    exit(0);
    return 0;
}
