#ifndef SCCWIRELESSRCVRPROTOCOL_H
#define SCCWIRELESSRCVRPROTOCOL_H


#include <vector>
#include <unordered_map>
#include <queue>
#include <cstring>

#define CMD_INVALID "Invalid"
#define CMD_CHECKSTATUS "CheckStatus"
#define CMD_ADDRESSSETTING "AddressSetting"
#define CMD_GETTAGDATA "GetTagData"

#define HOST_HEADER "HostHeader"
#define WGT_HEADER "WGTHeader"

#define ADDRESS_BYTE 'U'
#define ETX_BYTE '\3'

#define MAX_WGT_BUFFER_SIZE 512

#define MIN_WGT_DATA 8

#define STATUS_FAILURE              0x4e
#define STATUS_SUCCESS              0x59
#define STATUS_RQT_ADDR_SETTING     0x01
#define STATUS_APPLY_ADDR_INFO      0x02
#define STATUS_ADDR_RCV_SUCCESS     0x03
#define STATUS_ADDR_DIST_SETTING    0x04
#define STATUS_ADDR_SET_SUCCEEDS    0x05
#define STATUS_NO_TAG_DATA          0x06
#define STATUS_TAG_READ_SUCCEEDS    0x07
#define STATUS_TAG_DATA_READY       0x08
#define STATUS_IDLE                 0x09
#define STATUS_NO_BATTERY           0x0a

enum Host2WGTCommand
{
    Invalid = 0,
    StatusCheck,
    AddressSetting,
    GetTagData,
};

struct commandStruct
{
    char    command;
    char    addr;
    char    len;
    char    data[60];

    commandStruct(char cmd, char addr, char* resp, char len)
     : command(cmd), addr(addr), len(len)
    {
        memcpy(data, resp, len);
    }
};

struct ActionStruct
{
    std::string     strCmd;
    int             iTimeOut;
    bool            bNozzleActived;
    bool            bAlarm;
    bool            bFail;

    ActionStruct(const std::string& cmd, const int timeOut, bool nozzleActived, bool alarm, bool fail)
     : strCmd(cmd), iTimeOut(timeOut), bNozzleActived(nozzleActived), bAlarm(alarm), bFail(fail)
    {}
    ActionStruct() {}
};

struct TagDataStruct
{
    char    chTagData[MAX_WGT_BUFFER_SIZE];
    char    chLenData;

    TagDataStruct(const char* buffer, const char len) : chLenData(len)
    {
        chTagData[0] = '\0';
        if (chLenData > 0)
            memcpy(chTagData, buffer, len);
    }
};

class SCCWirelessRcvrProtocol
{
    public:
        SCCWirelessRcvrProtocol();
        virtual ~SCCWirelessRcvrProtocol();

        std::string convChar2Hex(char* buffer, char& len);
        std::string getStrCmdStatusCheck(int addr, char* buffer, char& len);
        std::string getStrCmdSetAddr(int addr, int newAddr, char* buffer, char& len);
        std::string getStrCmdGetTagId(int addr, char* buffer, char& len);

        bool getWGTResponse(char* buffer, char len, std::string& cmd, int& addr, char* resp, char& respLen);

        std::string getStrStatus(char status);

        bool nextAction(int addr, char* buffer, char& len, int& timeout);

    protected:

        unsigned char calcCRC(unsigned char* pFirst, unsigned char* pEnd);
        std::string getStrCmd(const std::string& cmd, int addr, int addr2, char* buffer, char& len);
        void moveBufferToLeft(char* pos, char offset);
        std::string getWGTCommand(char cmd);
        bool getWGTResponse(std::string& cmd, int& addr, char* resp, char& respLen);
        void addCommandToDvcMap(char cmd, char addr, char* resp, char len);

        bool nextActionFromStatus(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout);
        bool nextActionFromAddressSetting(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout);
        bool nextActionFromGetTagData(commandStruct& cmdSt, int addr, char* buffer, char& len, int& timeout);

        void addStatusToVector(char addr, commandStruct& cmdSt);

        void addTagDataToMap(commandStruct& cmdSt, char addr);

        void getCommandFromAction(ActionStruct& actionSt, char* buffer, char& len);

        void setAlarm(char addr);
        void setNozzleActivated(char addr);
        void setFail(char addr);
        void clearAlarm(char addr);
        void clearNozzleActivated(char addr);
        void clearFail(char addr);

        void setVector(char addr, std::vector<bool>& vect);
        void clearVector(char addr, std::vector<bool>& vect);
        bool isVector(char addr, std::vector<bool>& vect);


    private:

        int m_iAddress;
        char m_chBufferIn[MAX_WGT_BUFFER_SIZE];
        char* m_pLast;
        int m_iBufferSize;

        std::unordered_map <char, std::queue<commandStruct>> m_DeviceMap;

        std::vector<char> m_chStatusVector;
        std::unordered_map <char, TagDataStruct> m_TagDataMap;
        std::vector<bool> m_bAlarmVector;
        std::vector<bool> m_bFailVector;
        std::vector<bool> m_bNozzleActivedVector;
};

#endif // SCCWIRELESSRCVRPROTOCOL_H
