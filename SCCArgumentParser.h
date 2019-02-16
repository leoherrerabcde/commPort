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

    ArgumentStruct(const std::string& argName, ArgumentType type, ArgumentValueType valType)
    : strArgName(argName), argType(type), valueType(valType)
    {}
    ArgumentStruct() {}

    bool isDetected(const std::string& argName);
    bool getValue(const std::string& argName, bool& val);
    /*bool getValue(const std::string& argName, int& val);
    bool getValue(const std::string& argName, std::string& val);*/
};

class SCCArgumentParser
{
    public:
        SCCArgumentParser();
        virtual ~SCCArgumentParser();

        void add(const std::string& arg, ArgumentType type, ArgumentValueType valType);


    protected:

    private:
};

#endif // SCCARGUMENTPARSER_H
