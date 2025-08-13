//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <string>
#include <sstream>
#include <exception>
#include "HJMacros.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJException : public std::exception
{
protected:
    long        line;
    const char* typeName;
    std::string description;
    std::string source;
    const char* file;
    std::string fullDesc; // storage for char* returned by what()
public:
    /** Static definitions of error codes.
     @todo
         Add many more exception codes, since we want the user to be able
         to catch most of them.
    */
    enum ExceptionCodes {
        ERR_CANNOT_WRITE_TO_FILE,
        ERR_INVALID_STATE,
        ERR_INVALIDPARAMS,
        ERR_RENDERINGAPI_ERROR,
        ERR_DUPLICATE_ITEM,
        ERR_ITEM_NOT_FOUND = ERR_DUPLICATE_ITEM,
        ERR_FILE_NOT_FOUND,
        ERR_INTERNAL_ERROR,
        ERR_RT_ASSERTION_FAILED,
        ERR_NOT_IMPLEMENTED,
        ERR_INVALID_CALL
    };

    /** Default constructor.
    */
    HJException( int number, const std::string& description, const std::string& source );

    /** Advanced constructor.
    */
    HJException( int number, const std::string& description, const std::string& source, const char* type, const char* file, long line );

    /** Copy constructor.
    */
    HJException(const HJException& rhs);

    /// Needed for compatibility with std::exception
    ~HJException() throw() {}

    /** Returns a string with the full description of this error.
     @remarks
         The description contains the error number, the description
         supplied by the thrower, what routine threw the exception,
         and will also supply extra platform-specific information
         where applicable. For example - in the case of a rendering
         library error, the description of the error will include both
         the place in which HJ found the problem, and a text
         description from the 3D rendering library, if available.
    */
    const std::string& getFullDescription(void) const { return fullDesc; }

    /** Gets the source function.
    */
    const std::string &getSource() const { return source; }

    /** Gets source file name.
    */
    const char* getFile() const { return file; }

    /** Gets line number.
    */
    long getLine() const { return line; }

    /** Returns a string with only the 'description' field of this exception. Use
     getFullDescriptionto get a full description of the error including line number,
     error number and what function threw the exception.
    */
    const std::string &getDescription(void) const { return description; }

    /// Override std::exception::what
    const char* what() const throw() { return fullDesc.c_str(); }
};

/** Template struct which creates a distinct type for each exception code.
@note
This is useful because it allows us to create an overloaded method
for returning different exception types by value without ambiguity.
From 'Modern C++ Design' (Alexandrescu 2001).
*/
class HJUnimplementedException : public HJException
{
public:
    HJUnimplementedException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJFileNotFoundException : public HJException
{
public:
    HJFileNotFoundException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJIOException : public HJException
{
public:
    HJIOException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJInvalidStateException : public HJException
{
public:
    HJInvalidStateException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJInvalidParametersException : public HJException
{
public:
    HJInvalidParametersException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJItemIdentityException : public HJException
{
public:
    HJItemIdentityException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJInternalErrorException : public HJException
{
public:
    HJInternalErrorException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJRenderingAPIException : public HJException
{
public:
    HJRenderingAPIException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJRuntimeAssertionException : public HJException
{
public:
    HJRuntimeAssertionException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};
class HJInvalidCallException : public HJException
{
public:
    HJInvalidCallException(int inNumber, const std::string& inDescription, const std::string& inSource, const char* inFile, long inLine)
     : HJException(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
};

/** Class implementing dispatch methods in order to construct by-value
 exceptions of a derived type based just on an exception code.
@remarks
 This nicely handles construction of derived Exceptions by value (needed
 for throwing) without suffering from ambiguity - each code is turned into
 a distinct type so that methods can be overloaded. This allows HJ_EXCEPT
 to stay small in implementation (desirable since it is embedded) whilst
 still performing rich code-to-type mapping.
*/
class HJExceptionFactory
{
private:
    /// Private constructor, no construction
    HJExceptionFactory() {}
    static void _throwException(
        HJException::ExceptionCodes code, int number,
        const std::string& desc,
        const std::string& src, const char* file, long line)
    {
        switch (code)
        {
            case HJException::ERR_CANNOT_WRITE_TO_FILE:   throw HJIOException(number, desc, src, file, line);
            case HJException::ERR_INVALID_STATE:          throw HJInvalidStateException(number, desc, src, file, line);
            case HJException::ERR_INVALIDPARAMS:          throw HJInvalidParametersException(number, desc, src, file, line);
            case HJException::ERR_RENDERINGAPI_ERROR:     throw HJRenderingAPIException(number, desc, src, file, line);
            case HJException::ERR_DUPLICATE_ITEM:         throw HJItemIdentityException(number, desc, src, file, line);
            case HJException::ERR_FILE_NOT_FOUND:         throw HJFileNotFoundException(number, desc, src, file, line);
            case HJException::ERR_INTERNAL_ERROR:         throw HJInternalErrorException(number, desc, src, file, line);
            case HJException::ERR_RT_ASSERTION_FAILED:    throw HJRuntimeAssertionException(number, desc, src, file, line);
            case HJException::ERR_NOT_IMPLEMENTED:        throw HJUnimplementedException(number, desc, src, file, line);
            case HJException::ERR_INVALID_CALL:           throw HJInvalidCallException(number, desc, src, file, line);
            default:                                       throw HJException(number, desc, src, "HJException", file, line);
        }
    }
    public:
        static void throwException(
            HJException::ExceptionCodes code,
            const std::string& desc,
            const std::string& src, const char* file, long line)
        {
            _throwException(code, code, desc, src, file, line);
        }
};
 
#ifndef HJ_EXCEPT
#   define HJ_EXCEPT_3(code, desc, src)  HJ::HJExceptionFactory::throwException(code, desc, src, __FILE__, __LINE__)
#   define HJ_EXCEPT_2(code, desc)       HJ::HJExceptionFactory::throwException(code, desc, __FUNCTION__, __FILE__, __LINE__)
#   define HJ_EXCEPT_CHOOSER(arg1, arg2, arg3, arg4, ...) arg4
#   define HJ_EXPAND(x) x // MSVC workaround
#   define HJ_EXCEPT(...) HJ_EXPAND(HJ_EXCEPT_CHOOSER(__VA_ARGS__, HJ_EXCEPT_3, HJ_EXCEPT_2)(__VA_ARGS__))
#endif

NS_HJ_END