#ifndef SCCWIRELESSRCVRPROTOCOL_H
#define SCCWIRELESSRCVRPROTOCOL_H

#include <vector>
#include <unordered_map>

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

class SCCWirelessRcvrProtocol
{
    public:
        SCCWirelessRcvrProtocol();
        virtual ~SCCWirelessRcvrProtocol();

        std::string convChar2Hex(char* buffer, int& len);
        std::string getStrCmdStatusCheck(int addr, char* buffer, int& len);
        std::string getStrCmdSetAddr(int addr, int newAddr, char* buffer, int& len);
        std::string getStrCmdGetTagId(int addr, char* buffer, int& len);

        bool getWGTResponse(char* buffer, int len, std::string& cmd, int& addr, char* resp, int respLen);

        std::string getStrStatus(char status);

    protected:

        //unsigned char getCRC(const std::string& msg);
        unsigned char calcCRC(unsigned char* pFirst, unsigned char* pEnd);
        std::string getStrCmd(const std::string& cmd, int addr, int addr2, char* buffer, int& len);
        void moveBufferToLeft(char* pos, int offset);
        std::string getWGTCommand(char cmd);
        bool getWGTResponse(std::string& cmd, int& addr, char* resp, int respLen);

    private:

        int m_iAddress;
        char m_chBufferIn[MAX_WGT_BUFFER_SIZE];
        char* m_pLast;
        int m_iBufferSize;
};

#endif // SCCWIRELESSRCVRPROTOCOL_H
