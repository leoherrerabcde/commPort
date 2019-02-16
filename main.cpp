#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include "SCCCommPort.h"
#include "SCCWirelessRcvrProtocol.h"
#include "SCCRealTime.h"
#include "SCCArgumentParser.h"

using namespace std;

static bool st_bSendMsgView = false;
static bool st_bRcvMsgView  = true;

void printSendMsg()
{
}

int main(int argc, char* argv[])
{
    int nCount = 0;
    int nPort = 7;
    int baudRate = 9600;
    float fTimeFactor = 1.0;
    if (argc > 2)
    {
        nPort = std::stoi(argv[1]);
        baudRate = std::stoi(argv[2]);
        if (argc > 3)
            fTimeFactor = std::stof(argv[3]);
    }
    SCCCommPort commPort;
    SCCWirelessRcvrProtocol rcvrProtocol;
    SCCRealTime clock;

    commPort.openPort(nPort, baudRate);

    char bufferOut[255];
    char bufferIn[250];
    char len;
    int iAddr = 1;
    std::string msg;

    msg = rcvrProtocol.getStrCmdStatusCheck(iAddr, bufferOut, len);

    msg = rcvrProtocol.convChar2Hex(bufferOut, len);

    if (st_bSendMsgView)
        cout << "Message: " << msg << " sent." << std::endl;
    commPort.sendData(bufferOut, len);

    if (st_bSendMsgView)
        cout << "Waiting for response" << std::endl;
    msg = "";

    int iTimeOut;
    bool bNextAddr;
    char chLen = 0;
    do
    {
        bNextAddr = true;
        iTimeOut = 100;
        if (chLen > 0)
        {
            if (st_bSendMsgView)
            {
                msg = rcvrProtocol.convChar2Hex(bufferOut, chLen);
                cout << "Sending Message: " << msg << std::endl;
            }
            commPort.sendData(bufferOut, chLen);
            chLen = 0;
            iTimeOut = 20;
            bNextAddr = false;
        }
        if (commPort.isRxEvent() == true)
        {
            bNextAddr =false;
            int iLen;
            bool ret = commPort.getData(bufferIn, iLen);
            if (ret == true)
            {
                len = (char)iLen;
                /*if (st_bRcvMsgView)
                {
                    msg = rcvrProtocol.convChar2Hex(bufferIn, len);
                    cout << ++nCount << " Buffer In(Hex): [" << msg << "]. Buffer In(char): [" << bufferIn << "]" << std::endl;
                }*/
                std::string strCmd;
                char resp[256];
                int addr = 0;
                char respLen = 0;
                bool bIsValidResponse = rcvrProtocol.getWGTResponse(bufferIn, len, strCmd, addr, resp, respLen);
                bool bNextAction = false;
                if (bIsValidResponse == true)
                {
                    if (st_bRcvMsgView)
                    {
                        cout << ++nCount << " " << commPort.printCounter() << clock.getTimeStamp() << " Valid WGT Response" << std::endl;
                        if (strCmd == CMD_CHECKSTATUS)
                            cout << ++nCount << " WGT Status: " << rcvrProtocol.getStrStatus(resp[0]) << endl;
                    }
                    bNextAction = rcvrProtocol.nextAction(iAddr, bufferOut, chLen, iTimeOut);
                    if (bNextAction == true)
                        if (st_bRcvMsgView)
                            std::cout << ++nCount << " " << commPort.printCounter() << rcvrProtocol.printStatus(iAddr);
                }
            }
        }
        if (iTimeOut > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(iTimeOut*fTimeFactor)));
        if (bNextAddr == true)
        {
            //++iAddr;
        }
    }
    while (commPort.isOpened());

    //cout << "getData()= " << msg << std::endl;

    commPort.closePort();

    return 0;
}
