//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJException.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJException::HJException(int num, const std::string& desc, const std::string& src) :
    HJException(num, desc, src, "", "", 0)
{
}

HJException::HJException(int num, const std::string& desc, const std::string& src,
    const char* typ, const char* fil, long lin) :
    line( lin ),
    typeName(typ),
    description( desc ),
    source( src ),
    file( fil )
{
    std::stringstream ss;

    ss << typeName << ": "
       << description
       << " in " << source;

    if( line > 0 )
    {
        ss << " at " << file << " (line " << line << ")";
    }

    fullDesc = ss.str();
}

HJException::HJException(const HJException& rhs)
    : line( rhs.line ),
    typeName( rhs.typeName ),
    description( rhs.description ),
    source( rhs.source ),
    file( rhs.file )
{
}


NS_HJ_END