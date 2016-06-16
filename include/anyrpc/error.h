// Copyright (C) 2015 SRG Technology, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ANYRPC_ERROR_H_
#define ANYRPC_ERROR_H_

#include "api.h"
#include "logger.h"

namespace anyrpc
{

//! Error codes for system
/*!
 *  All of the AnyRPC error are consolidated to this list.
 *  The grouping is based on RPC recommendations (http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php).
 *  When parsing a document it is also useful to known where in the
 *  document the error occurred.  This can be supplied as an offset
 *  from the start of the document.
 */
enum ANYRPC_API AnyRpcErrorCode
{
    // Custom Server Errors
    AnyRpcErrorServerError                          = -32000,   //!< Generic server error
    AnyRpcErrorResponseParseError                   = -32001,   //!< Parse error with RPC response
    AnyRpcErrorInvalidResponse                      = -32002,   //!< Invalid RPC response

    // Transport Errors
    AnyRpcErrorTransportError                       = -32300,   //!< Generic transport error

    // System Errors
    AnyRpcErrorSystemError                          = -32400,   //!< Generic system error
    AnyRpcErrorValueAccess                          = -32401,   //!< Accessing a Value as the wrong type
    AnyRpcErrorStringNotTerminated                  = -32402,   //!< Supplied string was not null terminated but required
    AnyRpcErrorIllegalAssignment                    = -32403,   //!< Illegal assignment of Value
    AnyRpcErrorIllegalArrayAccess                   = -32404,   //!< Illegal access of array values
    AnyRpcErrorMemoryAllocation                     = -32405,   //!< Memory allocation failure
    AnyRpcErrorAccessInvalidValue                   = -32406,   //!< Access to Value that was set to invalid
    AnyRpcErrorIllegalCall                          = -32407,   //!< Illegal function call
    AnyRpcErrorBufferOverrun                        = -32408,   //!< Attempt to overrun buffer with insitu string
    AnyRpcErrorAccessNotInvalidValue                = -32409,   //!< Expected Value to be set to invalid but it is not
    AnyRpcErrorMapCountWrong                        = -32410,   //!< Map count is not correct
    AnyRpcErrorArrayCountWrong                      = -32411,   //!< Array count is not correct
    AnyRpcErrorShutdown                             = -32412,   //!< Illegal call to shutdown thread
    AnyRpcErrorPrettyPrintLevel                     = -32413,   //!< Error with the level for pretty printing xml
    AnyRpcErrorUnicodeValue                         = -32414,   //!< Illegal Unicode character
    AnyRpcErrorSurrogatePair                        = -32415,   //!< Invalid surrogate pair
    AnyRpcErrorUtf8Sequence                         = -32416,   //!< Invalid utf8 sequence
    AnyRpcErrorHandlerNotDefined                    = -32417,   //!< The RPC handler was not defined
    AnyRpcErrorNullInString                         = -32418,   //!< Null value in the middle of a string

    // Application Errors
    AnyRpcErrorApplicationError                     = -32500,   //!< Generic application error

    // Standard Server Errors
    AnyRpcErrorInvalidRequest                       = -32600,   //!< RPC request was not valid
    AnyRpcErrorMethodNotFound                       = -32601,   //!< RPC method requested is not defined
    AnyRpcErrorInvalidParams                        = -32602,   //!< RPC method parameters invalid
    AnyRpcErrorInternalError                        = -32603,   //!< RPC internal error
    AnyRpcErrorMethodRedefine                       = -32604,   //!< RPC attempt to redefine method
    AnyRpcErrorFunctionRedefine                     = -32605,   //!< RPC attempt to redefine function

    // Parse Errors
    AnyRpcErrorParseError                           = -32700,   //!< Generic parse error
    AnyRpcErrorUnsupportedEncoding                  = -32701,   //!< Character encoding not supported
    AnyRpcErrorInvalidEncoding                      = -32702,   //!< Character encoding invalid
    AnyRpcErrorDocumentEmpty                        = -32703,   //!< The document is empty.
    AnyRpcErrorDocumentRootNotSingular              = -32704,   //!< The document root must not follow by other values.
    AnyRpcErrorValueInvalid                         = -32705,   //!< Invalid value.
    AnyRpcErrorObjectMissName                       = -32706,   //!< Missing a name for object member.
    AnyRpcErrorObjectMissColon                      = -32707,   //!< Missing a colon after a name of object member.
    AnyRpcErrorObjectMissCommaOrCurlyBracket        = -32708,   //!< Missing a comma or '}' after an object member.
    AnyRpcErrorArrayMissCommaOrSquareBracket        = -32709,   //!< Missing a comma or ']' after an array element.
    AnyRpcErrorStringUnicodeEscapeInvalid           = -32710,   //!< Incorrect hex digit after \\u escape in string.
    AnyRpcErrorStringUnicodeSurrogateInvalid        = -32711,   //!< The surrogate pair in string is invalid.
    AnyRpcErrorStringEscapeInvalid                  = -32712,   //!< Invalid escape character in string.
    AnyRpcErrorStringMissingQuotationMark           = -32713,   //!< Missing a closing quotation mark in string.
    AnyRpcErrorStringInvalidEncoding                = -32714,   //!< Invalid encoding in string.
    AnyRpcErrorNumberTooBig                         = -32715,   //!< Number too big to be stored in double.
    AnyRpcErrorNumberMissFraction                   = -32716,   //!< Miss fraction part in number.
    AnyRpcErrorNumberMissExponent                   = -32717,   //!< Miss exponent in number.
    AnyRpcErrorTermination                          = -32718,   //!< Parsing was terminated.
    AnyRpcErrorNonspecificSyntaxError               = -32719,   //!< Non-specific syntax error.
    AnyRpcErrorTagInvalid                           = -32720,   //!< Parse error with xml tag
    AnyRpcErrorDateTimeInvalid                      = -32721,   //!< Invalid DateTime string
    AnyRpcErrorNotImplemented                       = -32722,   //!< Feature not implemented
    AnyRpcErrorHandler                              = -32723,   //!< Error when creating value in handler
    AnyRpcErrorBase64Invalid                        = -32724,   //!< Error during base64 decode

    AnyRpcErrorNone                                 = 0,        //!< No error value
};

//! Common exception class for AnyRPC
class ANYRPC_API AnyRpcException
{
public:
    //! Constructor set to no error
    AnyRpcException() : code_(AnyRpcErrorNone), offset_(0) {}
    //! Constructor for most exceptions
    AnyRpcException(int code, const std::string& message) :
        code_(code), message_(message), offset_(0) {}
    //! Constructor for parsing where the offset into the document can be specified
    AnyRpcException(int code, const std::string& message, std::size_t offset) :
        code_(code), message_(message), offset_(offset) {}

    const std::string& GetMessage() const { return message_;}
    int GetCode() const { return code_; }
    std::size_t GetOffset() const { return offset_; }
    void SetOffset(std::size_t offset) { offset_ = offset; }

    bool IsErrorSet() const { return (code_ != AnyRpcErrorNone); }
    void Clear() { code_ = AnyRpcErrorNone; }

private:
    int code_;                  //!< Code for the error
    std::string message_;       //!< Message associated with the error
    std::size_t offset_;        //!< Offset from the start of the document for a parse error
};

////////////////////////////////////////////////////////////////////////////////

#if (ANYRPC_ASSERT==2) && !defined(NDEBUG)
// assert will only be active in debug mode
# define anyrpc_assert(condition, code, message)                    \
    log_assert(condition, "Code: " << code << ", Message: " << message)

#elif (ANYRPC_ASSERT >= 1)
// throw if explicitly requested or if not debug and assert requested
# define anyrpc_assert(condition, code, message)                    \
    do {                                                            \
        if (!(condition))                                           \
        {                                                           \
            log_assert_message(condition,"Code: " << code << ", Message: " << message); \
            std::stringstream anyrpcAssertBuffer;                   \
            anyrpcAssertBuffer << message;                          \
            throw AnyRpcException( code, anyrpcAssertBuffer.str() );\
        }                                                           \
    } while (0)

#else
// ignore assert calls
# define anyrpc_assert(condition, code, message)

#endif

#if (ANYRPC_THROW==2) && !defined(NDEBUG)
// assert will only be active in debug mode
// perform an assert for anyrpc_throw to assist in debugging
# define anyrpc_throw(code, message)                                  \
    log_assert(false, "Throw exception, Code: " << code << ", Message: " << message)

#else
// throw exceptions
# define anyrpc_throw(code, message)                                  	\
    do {                                                                \
        log_error("Throw exception, Code: " << code << ", Message: " << message);\
        std::stringstream anyrpcAssertBuffer;                           \
        anyrpcAssertBuffer << message;                                  \
        throw AnyRpcException( code, anyrpcAssertBuffer.str() );        \
    } while (0)

#endif

} // namespace anyrpc

#endif // ANYRPC_ERROR_H_
