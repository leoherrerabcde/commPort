#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include "SCCCommPort.h"
#include "SCCWirelessRcvrProtocol.h"

using namespace std;

int main(int argc, char* argv[])
{
    cout << "Hello world!" << endl;

    int nPort = 7;
    int baudRate = 9600;
    if (argc > 2)
    {
        nPort = std::stoi(argv[1]);
        baudRate = std::stoi(argv[2]);
    }
    SCCCommPort commPort;
    SCCWirelessRcvrProtocol rcvrProtocol;


    commPort.openPort(nPort, baudRate);

    std::string msg("1234567890");
    /*cout << "Sending Message: " << msg << " for testing." << std::endl;
    commPort.sendData(msg);
    */
    char bufferOut[255];
    char bufferIn[250];
    int len;

    msg = rcvrProtocol.getStrCmdStatusCheck(1, bufferOut, len);

    msg = rcvrProtocol.convChar2Hex(bufferOut, len);

    cout << "Message: " << msg << " sent." << std::endl;
    commPort.sendData(bufferOut, len);

    cout << "Waiting for response" << std::endl;
    msg = "";
    do
    {
        if (commPort.isRxEvent() == true)
        {
            bool ret = commPort.getData(bufferIn, len);
            if (ret == true)
            {
                msg = rcvrProtocol.convChar2Hex(bufferIn, len);
                cout << "Buffer In(Hex): [" << msg << "]. Buffer In(char): [" << bufferIn << "]" << std::endl;
                std::string strCmd;
                char resp[256];
                int addr, respLen;
                bool bIsValidResponse = rcvrProtocol.getWGTResponse(bufferIn, len, strCmd, addr, resp, respLen);
                if (bIsValidResponse == true)
                {
                    cout << "Valid WGT Response";
                    if (strCmd == CMD_CHECKSTATUS)
                        cout << "WGT Status: " << rcvrProtocol.getStrStatus(resp[0]);
                }
            }
        }
         std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    while (commPort.isOpened());

    cout << "getData()= " << msg << std::endl;

    commPort.closePort();

    return 0;
}
