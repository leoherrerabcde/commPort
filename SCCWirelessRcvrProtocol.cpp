#include "SCCWirelessRcvrProtocol.h"

#include <sstream>
//#include <cstring>

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

std::unordered_map<char, std::string> stStatusMap =
{
    {0x4e, "General Failure"},
    {0x59, "Success"},
    {0x01, "Request Address Setting"},
    {0x02, "Applying Address Information"},
    {0x03, "Address Receiving succeeds"},
    {0x04, "Address Distributed and in Setting"},
    {0x05, "Address Setting Succeeds"},
    {0x06, "No tag data o tag data reading fails"},
    {0x07, "Tag Reading succeeds"},
    {0x08, "Tag Data Ready"},
    {0x09, "Idle State"},
    {0x0a, "Battery no Power"},
};

std::unordered_map<char, ActionStruct> stActionMap =
{
    {0x4e, {CMD_CHECKSTATUS, 1000, false,  true,  true}},
    {0x59, {CMD_CHECKSTATUS,  100, false, false, false}},
    {0x01, {CMD_CHECKSTATUS,  100, false, false, false}},
    {0x02, {CMD_CHECKSTATUS,  100, false, false, false}},
    {0x03, {CMD_CHECKSTATUS,  100, false, false, false}},
    {0x04, {CMD_CHECKSTATUS,  100, false, false, false}},
    {0x05, {CMD_CHECKSTATUS,  100, false, false, false}},
    {0x06, {CMD_CHECKSTATUS,   50,  true, false, false}},
    {0x07, { CMD_GETTAGDATA,   50,  true, false, false}},
    {0x08, { CMD_GETTAGDATA,   50,  true, false, false}},
    {0x09, {CMD_CHECKSTATUS,   50, false, false, false}},
    {0x0a, {CMD_CHECKSTATUS, 1000, false,  true,  true}},
};


SCCWirelessRcvrProtocol::SCCWirelessRcvrProtocol() : m_pLast(m_chBufferIn), m_iBufferSize(0), m_pCommandSt(NULL)
{
    //ctor
}

SCCWirelessRcvrProtocol::~SCCWirelessRcvrProtocol()
{
    //dtor
}

std::string SCCWirelessRcvrProtocol::getStrCmdStatusCheck(int addr,
                                               char* buffer,
                                               char& len)
{
    return getStrCmd(CMD_CHECKSTATUS, addr, 0, buffer, len);
}

std::string SCCWirelessRcvrProtocol::getStrCmdSetAddr(int addr,
                                               int addr2,
                                               char* buffer,
                                               char& len)
{
    return getStrCmd(CMD_ADDRESSSETTING, addr, addr2, buffer, len);
}

std::string SCCWirelessRcvrProtocol::getStrCmdGetTagId(int addr,
                                               char* buffer,
                                               char& len)
{
    return getStrCmd(CMD_GETTAGDATA, addr, 0, buffer, len);
}

std::string SCCWirelessRcvrProtocol::getStrCmd(const std::string& cmd,
                                               int addr,
                                               int addr2,
                                               char* buffer,
                                               char& len)
{
    char* p = buffer;
    char* pBCC;

    *p++ = stHeaderList[HOST_HEADER];
    *p++ = 0x03;
    pBCC = buffer;
    *p++ = stCmdList[cmd];
    *p++ = ADDRESS_BYTE;
    *p++ = (char)addr;
    if (addr2 >0)
    {
        *p++ = ADDRESS_BYTE;
        *p++ = (char) addr2;
    }
    *p++ = ETX_BYTE;
    unsigned char chBCC = calcCRC((unsigned char*)pBCC, (unsigned char*)p);
    *p++ = (char)chBCC;
    *p = '\0';
    len = p - buffer;

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

std::string SCCWirelessRcvrProtocol::convChar2Hex(char* buffer, char& len)
{
    std::stringstream ss;

    for (int i = 0; i < len; ++i)
        ss << std::hex << (int)(unsigned char)*buffer++ << " | ";
    //std::string msg;
    return std::string(ss.str());
}

bool SCCWirelessRcvrProtocol::getWGTResponse(std::string& cmd,
                                        int& addr,
                                        char* resp,
                                        char& respLen)
{
    bool bCmd = false;

    do
    {
        char chHeader = stHeaderList[WGT_HEADER];
        auto itHeader = m_chBufferIn;
        for (; itHeader != m_pLast && *itHeader != chHeader; ++itHeader);

        if (itHeader == m_pLast)
        {
            m_pLast = m_chBufferIn;
            m_iBufferSize = 0;
            return false;
        }
        if (itHeader !=m_chBufferIn)
        {
            char offset = itHeader - m_chBufferIn;
            moveBufferToLeft(itHeader, offset);
        }

        if (m_iBufferSize < MIN_WGT_DATA)
            return false;

        char chCmd, chAddr, chLen, *buf;

        char* p = m_chBufferIn + 1;
        respLen = chAddr = *p++;
        if (respLen + 4 > m_iBufferSize)
        {
            moveBufferToLeft(m_chBufferIn+1,1);
            continue;
        }

        cmd = getWGTCommand((chCmd = *p++));
        if (*p != ADDRESS_BYTE)
        {
            moveBufferToLeft(m_chBufferIn+1,1);
            continue;
        }
        p++;

        addr = chAddr = *p++;

        buf = p;
        memcpy(resp, p, respLen - 3);
        chLen -= 3;
        p += (respLen-3);
        if (*p != ETX_BYTE)
        {
            moveBufferToLeft(m_chBufferIn+1,1);
            continue;
        }
        ++p;
        unsigned char bcc = calcCRC((unsigned char*)m_chBufferIn, (unsigned char*)p);
        if ((unsigned char)*p != bcc)
        {
            moveBufferToLeft(m_chBufferIn+1,1);
            continue;
        }
        ++p;
        moveBufferToLeft(p, respLen + 4);
        addCommandToDvcMap(chCmd, chAddr, buf, chLen);
        bCmd = true;
    }
    while (bCmd == false && m_iBufferSize > 0);

    return bCmd;
}

bool SCCWirelessRcvrProtocol::getWGTResponse(char* buffer,
                                        char len,
                                        std::string& cmd,
                                        int& addr,
                                        char* resp,
                                        char& respLen)
{
    if (len <= 0)
        return false;

    int newLen = len;
    if (m_iBufferSize + len > MAX_WGT_BUFFER_SIZE)
        newLen = MAX_WGT_BUFFER_SIZE - m_iBufferSize;

    if (newLen <= 0)
    {
        m_pLast = m_chBufferIn;
        m_iBufferSize = 0;
        newLen = len;
    }

    memcpy(m_pLast, buffer, len);
    m_iBufferSize += len;
    m_pLast += len;

    return getWGTResponse(cmd, addr, resp, respLen);
}

std::string SCCWirelessRcvrProtocol::getWGTCommand(char cmd)
{
    for (auto it : stCmdList)
    {
        if (it.second == cmd)
            return it.first;
    }
    return CMD_INVALID;
}

void SCCWirelessRcvrProtocol::moveBufferToLeft(char* pos, char len)
{
    if (len == 0 || pos == m_chBufferIn)
        return;

    if (len >= m_iBufferSize || pos == m_pLast)
    {
        m_iBufferSize = 0;
        m_pLast = m_chBufferIn;
        return;
    }

    memcpy(m_chBufferIn, pos, m_iBufferSize - len);
    m_iBufferSize -= len;
    m_pLast -= len;
}

std::string SCCWirelessRcvrProtocol::getStrStatus(char status)
{
    return stStatusMap[status];
}

void SCCWirelessRcvrProtocol::addCommandToDvcMap(char cmd, char addr, char* resp, char len)
{
    m_pCommandSt = new commandStruct(cmd, addr, resp, len);

    /*if (m_DeviceVector.size() < addr)
    {
        for (char i = m_DeviceVector.size(); i < addr; ++i)
        {
            m_DeviceVector.push_back(NULL);
        }
    }
    m_DeviceVector[addr-1] = pCmdSt;*/
}

bool SCCWirelessRcvrProtocol::nextAction(int addr, char* buffer, char& len, int& timeout)
{
    commandStruct* pCmdSt = m_pCommandSt;

    if (pCmdSt== NULL)
        return false;

    bool res = false;
    if (pCmdSt->command == stCmdList[CMD_CHECKSTATUS])
    {
        res = nextActionFromStatus(*pCmdSt, addr, buffer, len, timeout);
    }
    else if (pCmdSt->command == stCmdList[CMD_ADDRESSSETTING])
    {
        res = nextActionFromAddressSetting(*pCmdSt, addr, buffer, len, timeout);
    }
    else if (pCmdSt->command == stCmdList[CMD_GETTAGDATA])
    {
        res = nextActionFromGetTagData(*pCmdSt, addr, buffer, len, timeout);
    }

    delete pCmdSt;
    return res;
}

bool SCCWirelessRcvrProtocol::nextActionFromStatus(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout)
{
    if (cmdSt.len <1)
        return false;

    //addStatusToVector(addr, cmdSt);
    ActionStruct actionSt = stActionMap[m_chStatusVector[addr-1]];
    getCommandFromAction(actionSt, buffer, len);
    timeout = actionSt.iTimeOut;
    if (actionSt.bAlarm)
        setAlarm(addr);
    if (actionSt.bFail)
        setFail(addr);
    if (actionSt.bNozzleActived)
        setNozzleActivated(addr);
    return true;
}

bool SCCWirelessRcvrProtocol::nextActionFromAddressSetting(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout)
{
    ActionStruct actionSt(CMD_CHECKSTATUS, 100, false, false, false);

    getCommandFromAction(actionSt, buffer, len);
    timeout = actionSt.iTimeOut;
    return true;
}

bool SCCWirelessRcvrProtocol::nextActionFromGetTagData(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout)
{
    addTagDataToMap(cmdSt, addr);
    return true;
}

void SCCWirelessRcvrProtocol::addStatusToVector(char addr, commandStruct& cmdSt)
{
    if (addr <1 || cmdSt.len < 1)
        return;

    if (m_chStatusVector.size() < addr)
        m_chStatusVector.resize(addr);
    m_chStatusVector[addr - 1] = cmdSt.data[0];
}

void SCCWirelessRcvrProtocol::addTagDataToMap(commandStruct& cmdSt, char addr)
{
    auto it = m_TagDataMap.find(addr);
    if (it ==m_TagDataMap.end())
    {
        TagDataStruct tagDataSt(cmdSt.data, cmdSt.len);
        m_TagDataMap.insert(std::make_pair(addr, tagDataSt));
    }
}

void SCCWirelessRcvrProtocol::getCommandFromAction(ActionStruct& actionSt, char* buffer, char& len)
{

}

void SCCWirelessRcvrProtocol::setAlarm(char addr)
{
    setVector(addr, m_bAlarmVector);
}

void SCCWirelessRcvrProtocol::setNozzleActivated(char addr)
{
    setVector(addr, m_bNozzleActivedVector);
}

void SCCWirelessRcvrProtocol::setFail(char addr)
{
    setVector(addr, m_bFailVector);
}

void SCCWirelessRcvrProtocol::clearAlarm(char addr)
{
    clearVector(addr, m_bAlarmVector);
}

void SCCWirelessRcvrProtocol::clearNozzleActivated(char addr)
{
    clearVector(addr, m_bNozzleActivedVector);
}

void SCCWirelessRcvrProtocol::clearFail(char addr)
{
    clearVector(addr, m_bFailVector);
}

void SCCWirelessRcvrProtocol::setVector(char addr, std::vector<bool>& vect)
{
    if (vect.size() < addr)
        vect.resize(addr);
    vect[addr-1] = true;
}

void SCCWirelessRcvrProtocol::clearVector(char addr, std::vector<bool>& vect)
{
    if (vect.size() < addr)
        vect.resize(addr);
    vect[addr-1] = false;
}

bool SCCWirelessRcvrProtocol::isVector(char addr, std::vector<bool>& vect)
{
    if (vect.size() < addr)
    {
        vect.resize(addr);
        vect[addr-1] = false;
    }
    return vect[addr-1];
}

