#ifndef SCCARGUMENTPARSER_H
#define SCCARGUMENTPARSER_H

#include <unordered_map>

enum ArgumentType
{
    Optional = 0,
    Required
};

enum ArgumentValueType
{
    Boolean = 0,
    Integer,
    Long,
    AlphaNumeric
};

struct ArgumentStruct
{
    std::string         strArgName;
    ArgumentType        argType;
    ArgumentValueType   valueType;
    bool                detected;
    std::string         value;

    ArgumentStruct() : detected(false)
    {}

    ArgumentStruct(const std::string& argName, ArgumentType type, ArgumentValueType valType)
    : strArgName(argName), argType(type), valueType(valType), detected(false)
    {}

    ArgumentStruct(const std::string& argName, ArgumentValueType valType, std::string val)
    : strArgName(argName), argType(Required), valueType(valType), detected(true), value(val)
    {}

    ArgumentStruct(const std::string& argName, ArgumentType type, ArgumentValueType valType, bool valid, std::string val)
    : strArgName(argName), argType(type), valueType(valType), detected(valid), value(val)
    {}

    bool isDetected(const std::string& argName);

    bool getValue(const std::string& argName, bool& val, const bool& valDefault = false);
    bool getValue(const std::string& argName, int& val, const int& valDefault = 0);
    bool getValue(const std::string& argName, std::string& val, const std::string& valDefault = "");
    /*bool getValue(const std::string& argName, int& val);
    bool getValue(const std::string& argName, std::string& val);*/
    bool getValue(int& val, const int& valDefault = 0)
    {
        if (!detected)
        {
            val = valDefault;
            return false;
        }
        val = std::stoi(value);
        return true;
    }
    bool getValue(std::string& val, const std::string& valDefault = 0)
    {
        if (!detected)
        {
            val = valDefault;
            return false;
        }
        val = value;
        return true;
    }
};

class SCCArgumentParser
{
    public:
        SCCArgumentParser();
        virtual ~SCCArgumentParser();

        void add(const std::string& arg, ArgumentType type, ArgumentValueType valType);
        void parser();
        bool isArgument(const std::string& arg);
        template <class T>
        bool getValue(const std::string& arg, T& value, T& valueDefault = false);

    protected:

    private:

        std::unordered_map<std::string, ArgumentStruct> m_ArgMap;
};

#endif // SCCARGUMENTPARSER_H
