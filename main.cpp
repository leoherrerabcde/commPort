#include <iostream>
#include <thread>
#include <chrono>

#include "SCCCommPort.h"

using namespace std;

int main()
{
    cout << "Hello world!" << endl;

    SCCCommPort commPort;

    commPort.openPort(7);

    std::string msg("1234567890");
    commPort.sendData(msg);

    cout << "Message: " << msg << " sent." << std::endl;

    cout << "Waiting for response" << std::endl;
    msg = "";
    do
    {
        msg = commPort.getData();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    while (msg == "");

    cout << "getData()= " << msg << std::endl;

    commPort.closePort();

    return 0;
}
