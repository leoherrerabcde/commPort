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
    if (argc > 1)
        nPort = std::stoi(argv[1]);
    SCCCommPort commPort;
    SCCWirelessRcvrProtocol rcvrProtocol;


    commPort.openPort(nPort);

    std::string msg("1234567890");
    /*cout << "Sending Message: " << msg << " for testing." << std::endl;
    commPort.sendData(msg);*/

    char bufferOut[255];
    char bufferIn[250];
    int len;

    msg = rcvrProtocol.getStrCmd(CMD_CHECKSTATUS, 1, bufferOut, len);

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
            //msg = commPort.getData(bufferIn, len);
            //cout << "Buffer In (str): " << msg << std::endl;
            if (ret == true)
            {
                msg = rcvrProtocol.convChar2Hex(bufferIn, len);
                cout << "Buffer In(char): " << bufferIn << std::endl;
            }
        }
         std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    while (commPort.isOpened());

    cout << "getData()= " << msg << std::endl;

    commPort.closePort();

    return 0;
}
