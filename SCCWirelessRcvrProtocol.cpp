#include "SCCWirelessRcvrProtocol.h"
#include "../main_control/SCCDeviceNames.h"

#include <sstream>
#include <iomanip>
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

static std::unordered_map<std::string, int> stVariableIndexMap=
{
    {VAR_BATTERY_ALARM  , BatteryAlarm},
    {VAR_FAIL_STATUS    , FailStatus},
    {VAR_NOZZLE_ACTIVED , NozzleActived},
    {VAR_TAG_DETECTED   , TagDetected},
};

static std::unordered_map<int,int> stVariableThresHoldMap =
{
    {BatteryAlarm   , 2},
    {FailStatus     , 2},
    {NozzleActived  , 3},
    {TagDetected    , 2},
};


SCCWirelessRcvrProtocol::SCCWirelessRcvrProtocol() : m_pLast(m_chBufferIn), m_iBufferSize(0)
{
    //ctor
    memset(m_bAlarmVector,0, sizeof(bool)*MAX_CHANNELS);
    memset(m_bFailVector,0, sizeof(bool)*MAX_CHANNELS);
    memset(m_bNozzleActivedVector,0, sizeof(bool)*MAX_CHANNELS);
    memset(m_bTagDetected,0, sizeof(bool)*MAX_CHANNELS);

    memset(m_VarStatus, 0, sizeof(VarStatus) * MAX_CHANNELS*VariableName_size);
    for (int addr = 0; addr < MAX_CHANNELS; ++addr)
        for (int var = 0; var < VariableName_size; ++var)
            m_VarStatus[addr][var].iThresHold   = stVariableThresHoldMap[var];
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
        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(unsigned char)*buffer++; //<< " | ";
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
        chLen = respLen - 3;
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
    if (MAX_CHANNELS < addr)
        return;

    commandStruct *pCmdSt = &m_DeviceVector[addr-1]; // (cmd, addr, resp, len);
    pCmdSt->set(cmd, addr, resp, len);
}

bool SCCWirelessRcvrProtocol::nextAction(int addr, char* buffer, char& len, int& timeout)
{
    if (MAX_CHANNELS < addr)
        return false;

    commandStruct CmdSt = m_DeviceVector[addr-1];

    bool res = false;

    if (CmdSt.command == stCmdList[CMD_CHECKSTATUS])
    {
        res = nextActionFromStatus(CmdSt, addr, buffer, len, timeout);
    }
    else if (CmdSt.command == stCmdList[CMD_ADDRESSSETTING])
    {
        res = nextActionFromAddressSetting(CmdSt, addr, buffer, len, timeout);
    }
    else if (CmdSt.command == stCmdList[CMD_GETTAGDATA])
    {
        res = nextActionFromGetTagData(CmdSt, addr, buffer, len, timeout);
    }

    return res;
}

bool SCCWirelessRcvrProtocol::nextActionFromStatus(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout)
{
    if (cmdSt.len <1)
        return false;

    addStatusToVector(addr, cmdSt);
    ActionStruct actionSt = getActionFromStatus(m_chStatusVector[addr-1]);
    getCommandFromAction(actionSt, addr, buffer, len);

    timeout = actionSt.iTimeOut;

    if (actionSt.bAlarm)
        setAlarm(addr);
    else
        clearAlarm(addr);

    if (actionSt.bFail)
        setFail(addr);
    else
        clearFail(addr);

    if (actionSt.bNozzleActived)
        setNozzleActivated(addr);
    else
    {
        clearNozzleActivated(addr);
        //clearTagDetected(addr);
    }

    if (getStatus(addr) != STATUS_TAG_READ_SUCCEEDS && getStatus(addr) != STATUS_TAG_DATA_READY)
        clearTagDetected(addr);
    /*else
    {
        //setTagDetected(addr);
        //std::cout << "Status Command: Tag Detected";
        return true;
    }*/
    return true;
}

char SCCWirelessRcvrProtocol::getStatus(char addr)
{
    if (addr > MAX_CHANNELS)
        return STATUS_FAILURE;
    return m_chStatusVector[addr-1];
}

bool SCCWirelessRcvrProtocol::nextActionFromAddressSetting(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout)
{
    ActionStruct actionSt(CMD_CHECKSTATUS, 1000, false, false, false);

    getCommandFromAction(actionSt, addr, buffer, len);
    timeout = actionSt.iTimeOut;
    return true;
}

bool SCCWirelessRcvrProtocol::nextActionFromGetTagData(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout)
{
    addTagDataToMap(cmdSt, addr);
    setTagDetected(addr);

    ActionStruct actionSt(CMD_CHECKSTATUS, 1000, false, false, false);

    getCommandFromAction(actionSt, addr, buffer, len);
    timeout = actionSt.iTimeOut;

    return true;
}

void SCCWirelessRcvrProtocol::addStatusToVector(char addr, commandStruct& cmdSt)
{
    if (addr <1 || cmdSt.len < 1)
        return;

    if (MAX_CHANNELS < addr)
        return;

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
    else
    {
        TagDataStruct tagDataSt(cmdSt.data, cmdSt.len);
        it->second = tagDataSt;
    }
}

void SCCWirelessRcvrProtocol::getCommandFromAction(ActionStruct& actionSt,char addr, char* buffer, char& len)
{
    if (actionSt.strCmd == CMD_CHECKSTATUS)
        getStrCmdStatusCheck(addr, buffer, len);
    else if (actionSt.strCmd == CMD_ADDRESSSETTING)
        getStrCmdSetAddr(addr, addr, buffer, len);
    else if (actionSt.strCmd == CMD_GETTAGDATA)
        getStrCmdGetTagId(addr, buffer, len);
    else
    {
        *buffer = NULL_CHAR;
        len = 0;
    }
}

void SCCWirelessRcvrProtocol::setVar(int addr, int var)
{
    VarStatus& v = m_VarStatus[addr][var];

    v.bCurrentStatus = true;
    v.iChangesCount = 0;
}

bool SCCWirelessRcvrProtocol::clearVar(int addr, int var)
{
    VarStatus& v = m_VarStatus[addr][var];

    if (v.bCurrentStatus == true)
    {
        ++v.iChangesCount;
        if (v.iChangesCount >= v.iThresHold)
        {
            v.bCurrentStatus = false;
            return true;
        }
    }
    return false;
}

bool SCCWirelessRcvrProtocol::isSetVar(int addr, int var)
{
    VarStatus& v = m_VarStatus[addr][var];

    return v.bCurrentStatus;
}


void SCCWirelessRcvrProtocol::setAlarm(char addr)
{
    setVector(addr, m_bAlarmVector);
    setVar(addr, BatteryAlarm);
}

void SCCWirelessRcvrProtocol::setNozzleActivated(char addr)
{
    setVector(addr, m_bNozzleActivedVector);
    setVar(addr, NozzleActived);
}

void SCCWirelessRcvrProtocol::setFail(char addr)
{
    setVector(addr, m_bFailVector);
    setVar(addr, FailStatus);
}

void SCCWirelessRcvrProtocol::setTagDetected(char addr)
{
    setVector(addr, m_bTagDetected);
    setVar(addr, TagDetected);
}


void SCCWirelessRcvrProtocol::clearAlarm(char addr)
{
    if (clearVar(addr, BatteryAlarm))
        clearVector(addr, m_bAlarmVector);
}

void SCCWirelessRcvrProtocol::clearNozzleActivated(char addr)
{
    if (clearVar(addr, NozzleActived))
        clearVector(addr, m_bNozzleActivedVector);
}

void SCCWirelessRcvrProtocol::clearFail(char addr)
{
    if (clearVar(addr, FailStatus))
        clearVector(addr, m_bFailVector);
}

void SCCWirelessRcvrProtocol::clearTagDetected(char addr)
{
    if (clearVar(addr, TagDetected))
        clearVector(addr, m_bTagDetected);
}

void SCCWirelessRcvrProtocol::setVector(char addr, bool* vect)
{
    if (MAX_CHANNELS < addr)
        return;
    vect[addr-1] = true;
}

void SCCWirelessRcvrProtocol::clearVector(char addr, bool* vect)
{
    if (MAX_CHANNELS < addr)
        return;
    vect[addr-1] = false;
}

bool SCCWirelessRcvrProtocol::isVector(char addr, bool* vect)
{
    if (MAX_CHANNELS < addr)
        return false;

    return vect[addr-1];
}

ActionStruct SCCWirelessRcvrProtocol::getActionFromStatus(char status)
{
    ActionStruct actionSt;
    switch(status)
    {
    case 0x4e:
        actionSt = {CMD_CHECKSTATUS, 1000, false,  true,  true};
        break;
    case 0x59:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x01:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x02:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x03:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x04:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x05:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x06:
        actionSt = {CMD_CHECKSTATUS, 1000,  true, false, false};
        break;
    case 0x07:
        actionSt = { CMD_GETTAGDATA, 1000,  true, false, false};
        break;
    case 0x08:
        actionSt = { CMD_GETTAGDATA, 1000,  true, false, false};
        break;
    case 0x09:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    case 0x0a:
        actionSt = {CMD_CHECKSTATUS, 1000, false,  true,  true};
        break;
    default:
        actionSt = {CMD_CHECKSTATUS, 1000, false, false, false};
        break;
    }
    return actionSt;
}

std::string SCCWirelessRcvrProtocol::printStatus(char addr)
{
    if (addr > MAX_CHANNELS)
        return "";

    std::stringstream ss;

    ss << FRAME_START_MARK ;
    ss << MSG_HEADER_TYPE << ASSIGN_CHAR << DEVICE_RFID_BOQUILLA;
    ss << SEPARATOR_CHAR << VAR_BATTERY_ALARM << ASSIGN_CHAR << boolToString(isAlarm(addr));
    ss << SEPARATOR_CHAR << VAR_FAIL_STATUS << ASSIGN_CHAR << boolToString(isFail(addr));
    ss << SEPARATOR_CHAR << VAR_NOZZLE_ACTIVED << ASSIGN_CHAR << boolToString(isNozzleActived(addr));
    ss << SEPARATOR_CHAR << VAR_TAG_DETECTED << ASSIGN_CHAR ;

    if (isTagDetected(addr))
    {
        char tagBuffer[MAX_WGT_BUFFER_SIZE];
        char lenTag;
        getTagId(addr, tagBuffer, lenTag);
        if (lenTag)
        {
            std::string strTag = convChar2Hex(tagBuffer, lenTag);
            ss << strTag;
        }
        else
            ss << boolToString(false);
    }
    else
    {
        ss << boolToString(isTagDetected(addr));
    }
    ss << FRAME_STOP_MARK;
    return std::string(ss.str());
}

bool SCCWirelessRcvrProtocol::isAlarm(char addr)
{
    return isVector(addr, m_bAlarmVector);
}

bool SCCWirelessRcvrProtocol::isFail(char addr)
{
    return isVector(addr, m_bFailVector);
}

bool SCCWirelessRcvrProtocol::isNozzleActived(char addr)
{
    return isVector(addr, m_bNozzleActivedVector);
}

bool SCCWirelessRcvrProtocol::isTagDetected(char addr)
{
    return isVector(addr, m_bTagDetected);
}

std::string SCCWirelessRcvrProtocol::boolToString(bool b, const std::string& valTrue, const std::string& valFalse)
{
    if (b ==true)
    {
        if (valTrue != "")
            return valTrue;
        return "true";
    }
    if (valFalse != "")
        return  valFalse;
    return "false";
}

bool SCCWirelessRcvrProtocol::getTagId(char addr, char* tagBuffer, char& len)
{
    auto it = m_TagDataMap.find(addr);
    if (it ==m_TagDataMap.end())
    {
        len = 0;
        tagBuffer[0] = NULL_CHAR;
        return false;
    }
    else
    {
        TagDataStruct& tagDataSt = it->second;
        len = tagDataSt.chLenData;
        memcpy(tagBuffer, tagDataSt.chTagData, tagDataSt.chLenData);
    }
    return true;
}
