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

        std::string getStrCmd(const std::string& cmd, int addr, char* buffer, int& len);
        std::string convChar2Hex(char* buffer, int& len);

    protected:

        //unsigned char getCRC(const std::string& msg);
        unsigned char calcCRC(unsigned char* pFirst, unsigned char* pEnd);

    private:

        int m_iAddress;
};

#endif // SCCWIRELESSRCVRPROTOCOL_H
