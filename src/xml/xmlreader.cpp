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

#include "anyrpc/api.h"
#include "anyrpc/logger.h"
#include "anyrpc/error.h"
#include "anyrpc/value.h"
#include "anyrpc/stream.h"
#include "anyrpc/handler.h"
#include "anyrpc/reader.h"
#include "anyrpc/xml/xmlreader.h"
#include "anyrpc/internal/strtod.h"
#include "anyrpc/internal/base64.h"

#include <cctype>

#define PARSE_ERROR_FOUND_EXPECTED(tag1,tag2)  "Parse error: found " << TagToString(tag1) << " expected " << TagToString(tag2)

namespace anyrpc
{
// set of tags and end tags
enum XmlTagEnum
{
    valueTag,           valueEndTag,           valueEmptyTag,           validExtraTag,
    booleanTag,         booleanEndTag,         booleanEmptyTag,         booleanExtraTag,
    doubleTag,          doubleEndTag,          doubleEmptyTag,          doubleExtraTag,
    intTag,             intEndTag,             intEmptyTag,             intExtraTag,
    i4Tag,              i4EndTag,              i4EmptyTag,              i4ExtraTag,
    i8Tag,              i8EndTag,              i8EmptyTag,              i8ExtraTag,
    stringTag,          stringEndTag,          stringEmptyTag,          stringExtraTag,
    dateTimeTag,        dateTimeEndTag,        dateTimeEmptyTag,        dataTimeExtraTag,
    base64Tag,          base64EndTag,          base64EmptyTag,          base64ExtraTag,
    nilTag,             nilEndTag,             nilEmptyTag,             nilExtraTag,
    arrayTag,           arrayEndTag,           arrayEmptyTag,           arrayExtraTag,
    dataTag,            dataEndTag,            dataEmptyTag,            dataExtraTag,
    structTag,          structEndTag,          structEmptyTag,          structExtraTag,
    memberTag,          memberEndTag,          memberEmptyTag,          memberExtraTag,
    nameTag,            nameEndTag,            nameEmptyTag,            nameExtraTag,
    methodCallTag,      methodCallEndTag,      methodCallEmptyTag,      methodCallExtraTag,
    methodNameTag,      methodNameEndTag,      methodNameEmptyTag,      methodNameExtraTag,
    methodResponseTag,  methodResponseEndTag,  methodResponseEmptyTag,  methodResponseExtraTag,
    paramsTag,          paramsEndTag,          paramsEmptyTag,          paramsExtraTag,
    paramTag,           paramEndTag,           paramEmptyTag,           paramExtraTag,
    faultTag,           faultEndTag,           faultEmptyTag,           faultExtraTga,
    invalidTag,         invalidEndTag,         invalidEmptyTag,         invalidExtraTag,
};

typedef struct
{
    int tagValue;           //!< enumeration value for the tag
    int tagLength;          //!< length of the tag to optimize comparison
    const char *tagName;    //!< string name for the tag
} xmlTag;

// must be in the same order as XmlTagEnum due to the way TagToString indexes
static const xmlTag xmlTagList[] =
{
        { valueTag,         5,  "value" },
        { booleanTag,       7,  "boolean" },
        { doubleTag,        6,  "double" },
        { intTag,           3,  "int" },
        { i4Tag,            2,  "i4" },
        { i8Tag,            2,  "i8" },
        { stringTag,        6,  "string" },
        { dateTimeTag,      16, "dateTime.iso8601" },
        { base64Tag,        6,  "base64" },
        { nilTag,           3,  "nil" },
        { arrayTag,         5,  "array" },
        { dataTag,          4,  "data" },
        { structTag,        6,  "struct" },
        { memberTag,        6,  "member" },
        { nameTag,          4,  "name" },
        { methodCallTag,    10, "methodCall" },
        { methodNameTag,    10, "methodName" },
        { methodResponseTag,14, "methodResponse" },
        { paramsTag,        6,  "params" },
        { paramTag,         5,  "param" },
        { faultTag,         5,  "fault" },
        { invalidTag,       0,  "" },
};

static const int MaxXmlTagLength = 100;     // the xml version string will be the longest

void XmlReader::ParseStream(Handler& handler)
{
    log_time(INFO);

    // setup for stream parsing
    handler_ = &handler;
    parseError_.Clear();

    handler.StartDocument();

    // Actually parse the stream for values
    try
    {
        SkipWhiteSpace();

        if (!is_.Eof())
        {
            ParseValue();
        }
    }
    catch (AnyRpcException &fault)
    {
        log_error("catch exception, stream offset=" << is_.Tell());
        fault.SetOffset( is_.Tell() );
        SetParseError(fault);
    }

    handler.EndDocument();
}

std::string XmlReader::ParseRequest(Handler& handler)
{
    log_time(INFO);
    std::string methodName;
    handler_ = &handler;
    parseError_.Clear();

    handler.StartDocument();
    bool methodNameFound = false;
    bool paramsFound = false;
    int tag;
    try
    {
        // skip tags until the methodCall tag
        // this could skip through some incorrectly formed xml declarations but these are not needed by xmlrpc
        while ((tag = GetNextTag()) != methodCallTag)
        {
        }

        // look for other tags
        while ((tag = GetNextTag()) != methodCallEndTag)
        {
            switch (tag)
            {
                case methodNameTag:
                {
                    // method name can only be defined once
                    if (methodNameFound)
                        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag) << ": methodName redefined");
                    methodName = ParseMethodName();
                    methodNameFound = true;
                    break;
                }
                case paramsTag:
                {
                    ParseParams();
                    paramsFound = true;
                    break;
                }
                case paramsEmptyTag:
                {
                    // params can only be defined once
                    if (paramsFound)
                        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag) << ": params tag redefined");
                    handler_->StartArray();
                    handler_->EndArray();
                    paramsFound = true;
                    break;
                }
                default:
                    anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag));
            }
        }

        // the method name must be defined
        if (!methodNameFound)
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag: methodName not defined");

        // the params can be left undefined to indicate no parameters
        if (!paramsFound)
        {
            handler_->StartArray();
            handler_->EndArray();
        }
    }
    catch (AnyRpcException &fault)
    {
        log_error("catch exception, stream offset=" << is_.Tell());
        fault.SetOffset( is_.Tell() );
        SetParseError(fault);
    }
    handler.EndDocument();
    return methodName;
}

void XmlReader::ParseResponse(Handler& handler)
{
    log_time(INFO);
    handler_ = &handler;
    parseError_.Clear();

    handler.StartDocument();

    try
    {
        int tag;
        // skip tags until the methodResponse tag
        // this could skip through some incorrectly formed xml declarations but these are not needed by xmlrpc
        while ((tag = GetNextTag()) != methodResponseTag)
        {
        }

        switch (tag = GetNextTag())
        {
            case paramsTag :
            {
                ParseParams();
                if (GetNextTag() != methodResponseEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,methodResponseEndTag));
                break;
            }
            case paramsEmptyTag :
            {
                handler_->Null();
                break;
            }
            case faultTag :
            {
                ParseValue();
                if ((tag = GetNextTag()) != faultEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,faultEndTag));
                if ((tag = GetNextTag()) != methodResponseEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,methodResponseEndTag));
                break;
            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag));
        }
    }
    catch (AnyRpcException &fault)
    {
        log_error("catch exception, stream offset=" << is_.Tell());
        fault.SetOffset( is_.Tell() );
        SetParseError(fault);
    }

    handler.EndDocument();
}

std::string XmlReader::ParseMethodName()
{
    WriteStringStream wsstream(DefaultParseReserve);

    // Parse to the end of the string
    ParseStringToStream(wsstream);

    // The next tag should be the methodName end tag
    int tag;
    if ((tag = GetNextTag()) != methodNameEndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,methodNameEndTag));

    if (wsstream.Length() == 0)
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag: method name must be defined");

    // return the method name
    return wsstream.GetString();
}

void XmlReader::ParseParams()
{
    handler_->StartArray();

    size_t elementCount = 0;
    int tag;
    while ((tag = GetNextTag()) != paramsEndTag)
    {
        if (tag == paramEmptyTag)
        {
            // allowed?
        }
        else if (tag == paramTag)
        {
            if (elementCount != 0)
                handler_->ArraySeparator();

            switch (tag = GetNextTag())
            {
                case valueTag      : ParseValue(true);   break;
                case valueEmptyTag : ParseEmptyString(); break;  // same as <value><value/>, empty string
                default            : anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,valueTag));
            }
            if ((tag = GetNextTag()) != paramEndTag)
                anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,paramsEndTag));
            elementCount++;
        }
        else
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag));
    }
    handler_->EndArray(elementCount);
    return;
}

std::string XmlReader::TagToString(int tag)
{
    log_trace();
    WriteStringStream wsstream(DefaultParseReserve);;

    int baseTag = std::max(0,std::min(invalidTag >> 2, tag >> 2));
    if (tag & 1)
    {
        wsstream.Put("</");
        wsstream.Put(xmlTagList[baseTag].tagName);
        wsstream.Put(">");
    }
    else if (tag & 2)
    {
        wsstream.Put("<");
        wsstream.Put(xmlTagList[baseTag].tagName);
        wsstream.Put("/>");
    }
    else
    {
        wsstream.Put("<");
        wsstream.Put(xmlTagList[baseTag].tagName);
        wsstream.Put(">");
    }

    return wsstream.GetString();
}

int XmlReader::GetNextTag(bool valueTagLast)
{
    log_trace();
    bool endTagMarkFound = false;
    bool emptyTagMarkFound = false;
    bool tagEndFound = false;

    if (!tagSkipFirstChar_)
    {
        if (valueTagLast)
            // if the next tag is valueEndTag, this will add the information as a string
            // if just whitespace, then return the next tag
            // otherwise, it will be an error
            return ParseString(valueTag);

        // there should only be whitespace to ignore before the starf of the tag
        SkipWhiteSpace();
        if (is_.Eof())
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated: expected < found EOF");

        char c = is_.Get();
        if (c != '<')
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag: expected < found " << c);
    }
    tagSkipFirstChar_ = false;
    // check for end tag marker
    if (is_.Peek() == '/')
    {
        is_.Get();
        endTagMarkFound = true;
    }
    // build tag string
    int tagLength = 0;
    char tagStr[MaxXmlTagLength];
    bool done = false;
    while (!is_.Eof() && !done)
    {
        char nextChar = is_.Get();
        tagStr[tagLength] = nextChar;
        if (nextChar == '>')
        {
            done = true;
        }
        else if (nextChar == '/')
        {
            if (endTagMarkFound || (is_.Get() != '>'))
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag: missing >");
            emptyTagMarkFound = true;
            done = true;
        }
        else if (IsWhiteSpace(nextChar))
        {
            // white space not allowed before a tag, i.e. < tag>
            if (tagLength == 0)
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag: white space before tag name");
            // ignore the rest of the data
            tagEndFound = true;
        }
        else if (!tagEndFound)
        {
            tagLength++;
            if (tagLength >= MaxXmlTagLength)
            {
                log_warn("GetNextTag: invalidTag, string too long");
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag: tag name too long");
            }
        }
    }
    tagStr[tagLength] = 0;

    if (!done)
    {
        log_warn("GetNextTag: invalidTag, end of file");
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated: expected > found EOF");
    }

    // search table for the tag
    int entry = 0;
    while (xmlTagList[entry].tagValue != invalidTag)
    {
        if ((tagLength == xmlTagList[entry].tagLength) &&
            (strncmp(tagStr,xmlTagList[entry].tagName,tagLength)) == 0)
        {
            int tag = xmlTagList[entry].tagValue;
            if (endTagMarkFound)
                tag+= 1;
            else if (emptyTagMarkFound)
                tag+= 2;
            log_debug("GetNextTag: " << TagToString(tag));
            return tag;
        }
        entry++;
    }

    // don't set a parse error
    log_info("GetNextTag: unknown tag, " << tagStr);
    return invalidTag;
}

void XmlReader::ParseValue(bool valueTagParsed)
{
    log_trace();
    int tag;
    if (!valueTagParsed)
    {
        switch (tag = GetNextTag())
        {
            case valueEmptyTag:
                // same as <value></value> so an empty string
                ParseEmptyString();
                return;
            case valueTag:
                break;
            default:
                anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,valueTag));
        }
    }

    switch (tag = GetNextTag(true))
    {
        case nilEmptyTag    : ParseNil();         break;
        case booleanTag     : ParseBoolean();     break;
        case intTag         : ParseNumber(tag);   break;
        case i4Tag          : ParseNumber(tag);   break;
        case i8Tag          : ParseNumber(tag);   break;
        case doubleTag      : ParseNumber(tag);   break;
        case stringTag      : ParseString(tag);   break;
        case stringEmptyTag : ParseEmptyString(); break;
        case arrayTag       : ParseArray();       break;
        case arrayEmptyTag  : ParseEmptyArray();  break;
        case structTag      : ParseMap();         break;
        case structEmptyTag : ParseEmptyMap();    break;
        case dateTimeTag    : ParseDateTime();    break;
        case base64Tag      : ParseBase64();      break;
        case base64EmptyTag : ParseEmptyBase64(); break;
        case valueEndTag    :                     break;  // string already added as part of GetNextTag()/ParseString()
        default             : anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag));
    }
    if (tag != valueEndTag)
    {
        if  ((tag = GetNextTag()) != valueEndTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,valueEndTag));
    }
}

void XmlReader::ParseNil()
{
    log_trace();
    handler_->Null();
}

void XmlReader::ParseBoolean()
{
    log_trace();
    if (is_.Eof())
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated: expected 0 or 1 found EOF");

    char c = is_.Get();
    switch (c)
    {
        case '0' :
            handler_->BoolFalse();
            break;
        case '1' :
            handler_->BoolTrue();
            break;
        default :
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated: expected 0 or 1 found " << c);
    }

    int tag;
    if ((tag = GetNextTag()) != booleanEndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,booleanEndTag));
}

// This function is adapted from RapidJason (https://github.com/miloyip/rapidjson)
// Allow exponential format for doubles
void XmlReader::ParseNumber(int tag)
{
    log_trace();
    // Parse minus
    bool minus = false;
    if (is_.Peek() == '-')
    {
        minus = true;
        is_.Get();
    }

    // Parse int: zero / ( digit1-9 *DIGIT )
    unsigned i = 0;
    uint64_t i64 = 0;
    bool use64bit = false;
    int significandDigit = 0;
    if (is_.Peek() == '0')
    {
        i = 0;
        is_.Get();
    }
    else if (isdigit(is_.Peek()))
    {
        i = static_cast<unsigned>(is_.Get() - '0');

        if (minus)
            while (isdigit(is_.Peek()))
            {
                if (i >= 214748364) { // 2^31 = 2147483648
                    if ((i != 214748364) || (is_.Peek() > '8'))
                    {
                        i64 = i;
                        use64bit = true;
                        break;
                    }
                }
                i = i * 10 + static_cast<unsigned>(is_.Get() - '0');
                significandDigit++;
            }
        else
            while (isdigit(is_.Peek()))
            {
                if (i >= 429496729) { // 2^32 - 1 = 4294967295
                    if ((i != 429496729) || (is_.Peek() > '5'))
                    {
                        i64 = i;
                        use64bit = true;
                        break;
                    }
                }
                i = i * 10 + static_cast<unsigned>(is_.Get() - '0');
                significandDigit++;
            }
    }
    else
        anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");

    // Parse 64bit int
    bool useDouble = false;
    double d = 0.0;
    if (use64bit)
    {
        if (minus)
            while (isdigit(is_.Peek()))
            {
                 if (i64 >= ANYRPC_UINT64_C2(0x0CCCCCCC, 0xCCCCCCCC)) // 2^63 = 9223372036854775808
                    if ((i64 != ANYRPC_UINT64_C2(0x0CCCCCCC, 0xCCCCCCCC)) || (is_.Peek() > '8'))
                    {
						d = static_cast<double>(i64);
                        useDouble = true;
                        break;
                    }
                i64 = i64 * 10 + static_cast<unsigned>(is_.Get() - '0');
                significandDigit++;
            }
        else
            while (isdigit(is_.Peek()))
            {
                if (i64 >= ANYRPC_UINT64_C2(0x19999999, 0x99999999)) // 2^64 - 1 = 18446744073709551615
                    if ((i64 != ANYRPC_UINT64_C2(0x19999999, 0x99999999)) || (is_.Peek() > '5'))
                    {
						d = static_cast<double>(i64);
                        useDouble = true;
                        break;
                    }
                i64 = i64 * 10 + static_cast<unsigned>(is_.Get() - '0');
                significandDigit++;
            }
    }

    // Force double for big integer
    if (useDouble) {
        while (isdigit(is_.Peek()))
        {
            if (d >= 1.7976931348623157e307) // DBL_MAX / 10.0
                anyrpc_throw(AnyRpcErrorNumberTooBig, "Number too big to be stored in double");
            d = d * 10 + (is_.Get() - '0');
        }
    }

    // Parse frac = decimal-point 1*DIGIT
    int expFrac = 0;
    if (is_.Peek() == '.')
    {
        is_.Get();

        if (!isdigit(is_.Peek()))
            anyrpc_throw(AnyRpcErrorNumberMissFraction, "Missing fraction part in number");

        if (!useDouble)
        {
#if ANYRPC_64BIT
            // Use i64 to store significand in 64-bit architecture
            if (!use64bit)
                i64 = i;

            while (isdigit(is_.Peek()))
            {
                if (i64 > ANYRPC_UINT64_C2(0x1FFFFF, 0xFFFFFFFF)) // 2^53 - 1 for fast path
                    break;
                else
                {
                    i64 = i64 * 10 + static_cast<unsigned>(is_.Get() - '0');
                    --expFrac;
                    if (i64 != 0)
                        significandDigit++;
                }
            }

            d = (double)i64;
#else
            // Use double to store significand in 32-bit architecture
            d = use64bit ? (double)i64 : (double)i;
#endif
            useDouble = true;
        }
        while (isdigit(is_.Peek()))
        {
            if (significandDigit < 17) {
                d = d * 10.0 + (is_.Get() - '0');
                --expFrac;
                if (d != 0.0)
                    significandDigit++;
            }
            else
                is_.Get();
        }
    }

    // Parse exp = e [ minus / plus ] 1*DIGIT
    int exp = 0;
    if ((is_.Peek() == 'e') || (is_.Peek() == 'E'))
    {
        if (!useDouble)
        {
			d = use64bit ? static_cast<double>(i64) : i;
            useDouble = true;
        }
        is_.Get();

        bool expMinus = false;
        if (is_.Peek() == '+')
            is_.Get();
        else if (is_.Peek() == '-')
        {
            is_.Get();
            expMinus = true;
        }

        if (isdigit(is_.Peek()))
        {
            exp = is_.Get() - '0';
            while (isdigit(is_.Peek()))
            {
                exp = exp * 10 + (is_.Get() - '0');
                if ((exp > 308) && !expMinus) // exp > 308 should be rare, so it should be checked first.
                    anyrpc_throw(AnyRpcErrorNumberTooBig, "Number too big to be stored in double");
            }
        }
        else
            anyrpc_throw(AnyRpcErrorNumberMissExponent, "Missing exponent in number");

        if (expMinus)
            exp = -exp;
    }

    // Finish parsing, call event according to the type of number.
    if (useDouble)
    {
        int p = exp + expFrac;
        d = internal::StrtodNormalPrecision(d, p);

        if (tag != doubleTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,doubleTag));
        if  ((tag = GetNextTag()) != doubleEndTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,doubleEndTag));
        handler_->Double(minus ? -d : d);
    }
    else {
        if (use64bit)
        {
            if ((tag != doubleTag) && (tag != i8Tag))
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error: found " << TagToString(tag) << " requires 64 bit (double/i8) tag");
            if (tag == doubleTag)
            {
                if ((tag = GetNextTag()) != doubleEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,doubleEndTag));
            }
            else
            {
                if ((tag = GetNextTag()) != i8EndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,i8EndTag));
            }
            if (minus)
                handler_->Int64(-(int64_t)i64);
            else
                handler_->Uint64(i64);
        }
        else
        {
            int nextTag = GetNextTag();
            if (nextTag != (tag+1))
                anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(nextTag,tag+1));
            if (minus)
                handler_->Int(-(int)i);
            else
                handler_->Uint(i);
        }
    }
}

int XmlReader::ParseString(int tag)
{
    log_trace();
    const char* str;
    size_t length;
    WriteStringStream wsstream(DefaultParseReserve);
    bool onlyWhiteSpace;

    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        str = is_.PutBegin();
        onlyWhiteSpace = ParseStringToStream(is_);
        is_.Get();
        is_.Put('\0'); // terminate the string
        length = is_.PutEnd() - 1;
        tagSkipFirstChar_ = true;
    }
    else
    {
        // When parsing to a new stream, need to create the local stream to write to.
        onlyWhiteSpace = ParseStringToStream(wsstream);
        length = wsstream.Length();
        str = wsstream.GetBuffer();
        is_.Get();
        tagSkipFirstChar_ = true;
    }
    switch (tag)
    {
        case stringTag:
            if ((tag = GetNextTag()) != stringEndTag)
                anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,stringEndTag));
            handler_->String(str,length,copy_);
            break;

        case valueTag:
            tag = GetNextTag();
            if (tag == valueEndTag)
                handler_->String(str,length,copy_);
            else if (!onlyWhiteSpace)
                anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,valueEndTag));
            break;

        case nameTag:
            if ((tag = GetNextTag()) != nameEndTag)
                anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,nameEndTag));
            handler_->Key(str,length,copy_);
            break;
    }
    return tag;
}

void XmlReader::ParseEmptyString()
{
    log_trace();
    const char* str = "";
    size_t length = 0;
    handler_->String(str,length,copy_);
}

void XmlReader::ParseKey()
{
    log_trace();
    int tag;
    if ((tag = GetNextTag()) != nameTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,nameTag));

    ParseString(nameTag);
}

static const char rawEntity[]  = {  '<',  '>',   '&',   '\'',   '\"', 0 };
static const char* xmlEntity[] = { "lt", "gt", "amp", "apos", "quot", 0 };

bool XmlReader::ParseStringToStream(Stream& os)
{
    bool onlyWhiteSpace = true;

    log_trace();
    while (!is_.Eof())
    {
        char c = is_.Peek();
        if (c == '<') // start of new tag
            return onlyWhiteSpace;
        if (c == '&') // Escape
        {
            onlyWhiteSpace = false;
            is_.Get();
            // find the encoded sequence
            int encodedLength = 0;
            char encoded[MaxXmlTagLength];
            bool done = false;
            while (!done)
            {
                if (is_.Eof())
                    return onlyWhiteSpace;
                encoded[encodedLength] = is_.Get();
                if (encoded[encodedLength] == ';')
                {
                    encoded[encodedLength] = 0;
                    log_info("encoded sequence = " << encoded );
                    done = true;
                }
                else
                {
                    encodedLength++;
                    if (encodedLength >= MaxXmlTagLength)
                        anyrpc_throw(AnyRpcErrorStringEscapeInvalid, "Invalid escape character in string");
                }
            }
            if (encodedLength == 0)
                anyrpc_throw(AnyRpcErrorStringEscapeInvalid, "Invalid escape character in string");
            if (encoded[0] == '#')
            {
                if ((encodedLength > 2) && (encoded[1] == 'x'))
                    ParseHexEscapeCode(os, &encoded[2], encodedLength-2);
                else
                    ParseDecimalEscapeCode(os, &encoded[1], encodedLength-1);
            }
            else
            {
                // search for the encoded sequence
                int entry = 0;
                while (true)
                {
                    if (xmlEntity[entry] == 0)
                        anyrpc_throw(AnyRpcErrorStringEscapeInvalid, "Invalid escape character in string");
                    if (strncmp(xmlEntity[entry], encoded, encodedLength) == 0)
                    {
                        // translate the sequence
                        os.Put( rawEntity[entry] );
                        break;
                    }
                    entry++;
                }
            }
        }
        else
        {
            char c = is_.Get();
            onlyWhiteSpace &= IsWhiteSpace(c);
            os.Put(c);
        }
    }
    anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");
}

void XmlReader::ParseHexEscapeCode(Stream& os, const char* str, int length)
{
    // hex escape code
    unsigned codepoint = 0;
    if (length > 5)
        anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid, "Unicode escape sequence invalid");

    for (int i=0; i<length; i++)
    {
        char c = str[i];
        codepoint <<= 4;
        if ((c >= '0') && (c <= '9'))
            codepoint += (c - '0');
        else if ((c >= 'A') && (c <= 'F'))
            codepoint += (c - 'A' + 10);
        else if ((c >= 'a') && (c <= 'f'))
            codepoint += (c - 'a' + 10);
        else
            anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid, "Unicode escape sequence invalid");
    }
    if (codepoint > 0x10FFFF)
        anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid, "Unicode escape sequence invalid");

    EncodeUtf8(os, codepoint);
}

void XmlReader::ParseDecimalEscapeCode(Stream& os, const char* str, int length)
{
    // decimal escape code
    unsigned codepoint = 0;
    if (length > 7)
        anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid, "Unicode escape sequence invalid");

    for (int i=0; i<length; i++)
    {
        char c = str[i];
        codepoint *= 10;
        if ((c >= '0') && (c <= '9'))
            codepoint += (c - '0');
        else
            anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid, "Unicode escape sequence invalid");
    }
    if (codepoint > 0x10FFFF)
        anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid, "Unicode escape sequence invalid");

    EncodeUtf8(os, codepoint);
}

// This function is adapted from RapidJason (https://github.com/miloyip/rapidjson)
void XmlReader::EncodeUtf8(Stream& os, unsigned codepoint)
{
    if (codepoint <= 0x7F)
    {
        os.Put(codepoint);
    }
    else if (codepoint <= 0x7FF)
    {
        os.Put(0xC0 | ((codepoint >> 6) & 0xFF));
        os.Put(0x80 | ((codepoint     ) & 0x3F));
    }
    else if (codepoint <= 0xFFFF)
    {
        os.Put(0xE0 | ((codepoint >>12) & 0xFF));
        os.Put(0x80 | ((codepoint >> 6) & 0x3F));
        os.Put(0x80 | ((codepoint     ) & 0x3F));
    }
    else
    {
        os.Put(0xF0 | ((codepoint >>18) & 0xFF));
        os.Put(0x80 | ((codepoint >>12) & 0x3F));
        os.Put(0x80 | ((codepoint >> 6) & 0x3F));
        os.Put(0x80 | ((codepoint     ) & 0x3F));
    }
}

void XmlReader::ParseMap()
{
    log_trace();
    handler_->StartMap();

    for (size_t memberCount = 0; ;memberCount++)
    {
        int tag = GetNextTag();
        switch (tag)
        {
            case memberTag :
            {
                if (memberCount != 0)
                    handler_->MapSeparator();

                ParseKey();
                ParseValue();
                if ((tag = GetNextTag()) != memberEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,memberEndTag));
                break;
            }
            case structEndTag :
            {
                handler_->EndMap(memberCount);
                return;

            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag));
        }
    }
}

void XmlReader::ParseEmptyMap()
{
    log_trace();
    handler_->StartMap();
    handler_->EndMap(0);
}

void XmlReader::ParseArray()
{
    log_trace();
    int tag;
    if ((tag = GetNextTag()) == dataEmptyTag)
    {
        // handle an empty data tag : <array><data/></array>
        handler_->StartArray();
        handler_->EndArray(0);
        if ((tag = GetNextTag()) != arrayEndTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,arrayEndTag));
        return;
    }
    if (tag != dataTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,dataTag));

    handler_->StartArray();

    for (size_t elementCount = 0; ;elementCount++)
    {
        switch ((tag = GetNextTag()))
        {
            case valueTag :
            {
                if (elementCount != 0)
                    handler_->ArraySeparator();
                ParseValue(true);
                break;
            }
            case valueEmptyTag :
            {
                ParseEmptyString();  // same as <value><value/> so an empty string
                break;
            }
            case dataEndTag :
            {
                handler_->EndArray(elementCount);
                if ((tag = GetNextTag()) != arrayEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,arrayEndTag));
                return;
            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag " << TagToString(tag));
        }
    }
}

void XmlReader::ParseEmptyArray()
{
    log_trace();
    // handle an empty array tag : </array>
    handler_->StartArray();
    handler_->EndArray(0);
}

void XmlReader::ParseDateTime()
{
    log_trace();
    WriteStringStream wsstream(DefaultParseReserve);
    ParseStringToStream(wsstream);
    size_t length = wsstream.Length();
    const char* str = wsstream.GetBuffer();

    int tag = GetNextTag();
    if (tag != dateTimeEndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,dateTimeEndTag));

    // convert string to time
    struct tm t;
    if ((length != 17) ||
        (sscanf(str, "%4d%2d%2dT%2d:%2d:%2d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6))
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated: failed to convert string to datetime");

    t.tm_year -= 1900;
    t.tm_mon -= 1;
    t.tm_isdst = -1;
    handler_->DateTime(mktime(&t));
}

void XmlReader::ParseBase64()
{
    log_trace();
    const char* str;
    size_t length;
    bool decodeResult;
    WriteStringStream wsstream(DefaultParseReserve);

    if (inSitu_)
    {
        str = is_.PutBegin();
        decodeResult = internal::Base64Decode(is_, is_, '<');
        length = is_.PutEnd();
    }
    else
    {
        decodeResult = internal::Base64Decode(wsstream, is_, '<');
        length = wsstream.Length();
        str = wsstream.GetBuffer();
    }
    if (!decodeResult)
        anyrpc_throw(AnyRpcErrorBase64Invalid, "Error during base64 decode");

    int tag = GetNextTag();
    if (tag != base64EndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, PARSE_ERROR_FOUND_EXPECTED(tag,base64EndTag));

    handler_->Binary( (const unsigned char*)str, length, copy_);
}

void XmlReader::ParseEmptyBase64()
{
    log_trace();
    const char* str = "";
    size_t length = 0;
    handler_->Binary( (const unsigned char*)str, length, copy_);
}
}
