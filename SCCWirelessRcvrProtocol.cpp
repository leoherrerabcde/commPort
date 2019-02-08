#include "SCCWirelessRcvrProtocol.h"

#include <sstream>

std::vector<std::string> stH2WCmdNameList =
{
    CMD_INVALID, CMD_CHECKSTATUS, CMD_ADDRESSSETTING, CMD_GETTAGDATA
};

std::unordered_map<std::string, char> stHeaderList =
{
    {HOST_HEADER,'H'},
    {WGT_HEADER,'W'},
};

std::unordered_map<std::string, char> stCmdList(
{
    {CMD_INVALID,' '},
    {CMD_CHECKSTATUS,'0'},
    {CMD_ADDRESSSETTING,'@'},
    {CMD_GETTAGDATA,'P'}
});

SCCWirelessRcvrProtocol::SCCWirelessRcvrProtocol()
{
    //ctor
}

SCCWirelessRcvrProtocol::~SCCWirelessRcvrProtocol()
{
    //dtor
}

std::string SCCWirelessRcvrProtocol::getStrCmd(const std::string& cmd,
                                               int addr,
                                               char* buffer,
                                               int& len)
{
    //unsigned char buf[256];

    char* p = buffer;
    char* pBCC;

    *p++ = stHeaderList[HOST_HEADER];
    *p++ = 0x03;
    pBCC = buffer;
    *p++ = stCmdList[cmd];
    *p++ = ADDRESS_BYTE;
    *p++ = (char)addr;
    *p++ = ETX_BYTE;
    unsigned char chBCC = calcCRC((unsigned char*)pBCC, (unsigned char*)p);
    *p++ = (char)chBCC;
    *p = '\0';
    len = p - buffer;
    /*std::string msg = std::string(stCmdList[cmd]);
    msg += std::string(ADDRESS_BYTE);
    msg += std::string(char(addr));
    msg += std::string(ETX_BYTE);
    msg += std::string(chBCC);*/

    std::string msg((char*)buffer);

    return msg;
}

unsigned char SCCWirelessRcvrProtocol::calcCRC(unsigned char* pFirst, unsigned char* pEnd)
{
    unsigned char ucBCC = '\0';

    for (unsigned char* p = pFirst; p != pEnd; ++p)
        ucBCC ^= *p;

    return ucBCC;
}

std::string SCCWirelessRcvrProtocol::convChar2Hex(char* buffer, int& len)
{
    std::stringstream ss;

    for (int i = 0; i < len; ++i)
        ss << std::hex << (int)(unsigned char)*buffer++ << " | ";
    //std::string msg;
    return std::string(ss.str());
}
