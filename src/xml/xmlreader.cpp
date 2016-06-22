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

namespace anyrpc
{
// set of tags and end tags
enum XmlTagEnum
{
    invalidTag = -1,
    valueTag,           valueEndTag,           valueEmptyTag,
    booleanTag,         booleanEndTag,         booleanEmptyTag,
    doubleTag,          doubleEndTag,          doubleEmptyTag,
    intTag,             intEndTag,             intEmptyTag,
    i4Tag,              i4EndTag,              i4EmptyTag,
    i8Tag,              i8EndTag,              i8EmptyTag,
    stringTag,          stringEndTag,          stringEmptyTag,
    emptyTag,           emptyEndTag,           emptyEmptyTag,
    dateTimeTag,        dateTimeEndTag,        dateTimeEmptyTag,
    base64Tag,          base64EndTag,          base64EmptyTag,
    nilTag,             nilEndTag,             nilEmptyTag,
    arrayTag,           arrayEndTag,           arrayEmptyTag,
    dataTag,            dataEndTag,            dataEmptyTag,
    structTag,          structEndTag,          structEmptyTag,
    memberTag,          memberEndTag,          memberEmptyTag,
    nameTag,            nameEndTag,            nameEmptyTag,
    methodCallTag,      methodCallEndTag,      methodCallEmptyTag,
    methodNameTag,      methodNameEndTag,      methodNameEmptyTag,
    methodResponseTag,  methodResponseEndTag,  methodResponseEmptyTag,
    paramsTag,          paramsEndTag,          paramsEmptyTag,
    paramTag,           paramEndTag,           paramEmptyTag,
    faultTag,           faultEndTag,           faultEmptyTag,
};

typedef struct
{
    int tagValue;           //!< enumeration value for the tag
    int tagLength;          //!< length of the tag to optimize comparison
    const char *tagName;    //!< string name for the tag
} xmlTag;

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
        { invalidTag,       0,  "" }

};

static const int MaxXmlTagLength = 100;     // the xml version string will be the longest

void XmlReader::ParseStream(Handler& handler)
{
    log_time(INFO);

    // setup for stream parsing
    handler_ = &handler;
    parseError_.Clear();

    handler.StartDocument();

    // Actually parse the stream.  This function may be called recursively.
    try
    {
        ParseStream();
    }
    catch (AnyRpcException &fault)
    {
        log_error("catch exception, stream offset=" << is_.Tell());
        fault.SetOffset( is_.Tell() );
        SetParseError(fault);
    }

    handler.EndDocument();
}

std::string XmlReader::ParseMethod()
{
    WriteStringStream wsstream(DefaultParseReserve);
    try
    {
        parseError_.Clear();

        // skip through data to find the tag: methodName
        int tag;
        while ((tag = GetNextTag()) != methodNameTag)
        {
            if (tag == emptyTag)
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
        }

        // Parse to the end of the string
        ParseStringToStream(wsstream);

        // The next tag should be the methodName end tag
        if (GetNextTag() != methodNameEndTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
    }
    catch (AnyRpcException &fault)
    {
        log_error("catch exception, stream offset=" << is_.Tell());
        fault.SetOffset( is_.Tell() );
        SetParseError(fault);
        return "";
    }

    // return the method name
    return wsstream.GetString();
}

void XmlReader::ParseParams(Handler& handler)
{
    log_time(INFO);
    handler_ = &handler;
    parseError_.Clear();

    handler.StartDocument();

    try
    {
        ParseParams();
    }
    catch (AnyRpcException &fault)
    {
        log_error("catch exception, stream offset=" << is_.Tell());
        fault.SetOffset( is_.Tell() );
        SetParseError(fault);
    }

    handler.EndDocument();
}

void XmlReader::ParseResponse(Handler& handler)
{
    log_time(INFO);
    handler_ = &handler;
    parseError_.Clear();

    handler.StartDocument();

    try
    {
        GetNextTag();  // should be <?xml version="1.0"?> but not currently checked
        if (GetNextTag() != methodResponseTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");

        switch (GetNextTag())
        {
            case paramsTag :
            {
                ParseParams(true);
                if (GetNextTag() != methodResponseEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
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
                if ((GetNextTag() != faultEndTag) &&
                    (GetNextTag() != methodResponseEndTag))
                    anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
                break;
            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
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

void XmlReader::ParseParams(bool paramsTagParsed)
{
    handler_->StartArray();

    if (!paramsTagParsed)
    {
        int nextTag = GetNextTag();
        if (nextTag == paramsEmptyTag)
        {
            // same as <params></params> so use an array with 0 elements
            handler_->EndArray();
            return;
        }
        if (nextTag != paramsTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
    }

    size_t elementCount = 0;
    while (true)
    {
        switch (GetNextTag())
        {
            case paramTag :
            {
                if (elementCount != 0)
                    handler_->ArraySeparator();

                switch (GetNextTag())
                {
                    case valueTag      : ParseValue(true);   break;
                    case valueEmptyTag : ParseEmptyString(); break;  // same as <value><value/>, empty string
                    default            : anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
                }
                if (GetNextTag() != paramEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
                break;
            }
            case paramsEndTag :
            {
                handler_->EndArray(elementCount);
                return;
            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
        }
        elementCount++;
    }
}

void XmlReader::ParseStream()
{
    log_trace();
    SkipWhiteSpace();

    if (!is_.Eof())
    {
        ParseValue();
    }
}

int XmlReader::GetNextTag()
{
    log_trace();
    bool endTagFound = false;
    bool emptyTagFound = false;

    if (!tagSkipFirstChar_)
    {
        SkipWhiteSpace();
        // check for start of tag
        if (is_.Peek() != '<')
        {
            log_debug("GetNextTag: emptyTag");
            return emptyTag;  // string
        }
        is_.Get();
    }
    tagSkipFirstChar_ = false;
    // check for end tag marker
    if (is_.Peek() == '/')
    {
        is_.Get();
        endTagFound = true;
    }
    // build tag string
    int tagLength = 0;
    char tag[MaxXmlTagLength];
    bool done = false;
    while (!is_.Eof() && !done)
    {
        tag[tagLength] = is_.Get();
        if (tag[tagLength] == '>')
        {
            tag[tagLength] = 0;
            done = true;
        }
        else if (tag[tagLength] == '/')
        {
            if (endTagFound || (is_.Get() != '>'))
            {
                log_warn("GetNextTag: invalidTag, missing >");
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
            }
            emptyTagFound = true;
            done = true;
        }
        else
        {
            tagLength++;
            if (tagLength >= MaxXmlTagLength)
            {
                log_warn("GetNextTag: invalidTag, string too long");
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
            }
        }
    }

    if (!done)
    {
        log_warn("GetNextTag: invalidTag, end of file");
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    }

    // search table for the tag
    int entry = 0;
    while (xmlTagList[entry].tagValue != invalidTag)
    {
        if ((tagLength == xmlTagList[entry].tagLength) &&
            (strncmp(tag,xmlTagList[entry].tagName,tagLength)) == 0)
        {
            int tagValue = xmlTagList[entry].tagValue;
            if (endTagFound)
                tagValue+= 1;
            else if (emptyTagFound)
                tagValue+= 2;
            log_debug("GetNextTag: " << xmlTagList[entry].tagName << ", " << tagValue );
            return tagValue;
        }
        entry++;
    }

    // don't set a parse error
    log_info("GetNextTag: unknown tag, " << tag);
    return invalidTag;
}

void XmlReader::ParseValue(bool valueTagParsed)
{
    log_trace();
    int tag;
    if (!valueTagParsed)
    {
        tag = GetNextTag();
        if (tag == valueEmptyTag)
        {
            // same as <value></value> so an empty string
            ParseEmptyString();
            return;
        }
        if (tag != valueTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
    }
    tag = GetNextTag();
    switch (tag)
    {
        case nilEmptyTag    : ParseNil();         break;
        case booleanTag     : ParseBoolean();     break;
        case intTag         : ParseNumber(tag);   break;
        case i4Tag          : ParseNumber(tag);   break;
        case i8Tag          : ParseNumber(tag);   break;
        case doubleTag      : ParseNumber(tag);   break;
        case stringTag      : ParseString(tag);   break;
        case stringEmptyTag : ParseEmptyString(); break;
        case emptyTag       : ParseString(tag);   break;
        case arrayTag       : ParseArray();       break;
        case arrayEmptyTag  : ParseEmptyArray();  break;
        case structTag      : ParseMap();         break;
        case structEmptyTag : ParseEmptyMap();    break;
        case dateTimeTag    : ParseDateTime();    break;
        case base64Tag      : ParseBase64();      break;
        case base64EmptyTag : ParseEmptyBase64(); break;
        case valueEndTag    : ParseEmptyString(); break;
        default             : anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");;
    }
    if ((tag != valueEndTag) && (GetNextTag() != valueEndTag))
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
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
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

    switch (is_.Get())
    {
        case '0' :
            if (GetNextTag() != booleanEndTag)
                anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
            handler_->BoolFalse();
            break;
        case '1' :
            if (GetNextTag() != booleanEndTag)
                anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
            handler_->BoolTrue();
            break;
        default :
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    }
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

        if ((tag != doubleTag) || (GetNextTag() != doubleEndTag))
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
        handler_->Double(minus ? -d : d);
    }
    else {
        if (use64bit)
        {
            if (((tag != doubleTag) && (tag != i8Tag)) || (GetNextTag() != (tag+1)))
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
            if (minus)
                handler_->Int64(-(int64_t)i64);
            else
                handler_->Uint64(i64);
        }
        else
        {
            if (GetNextTag() != (tag+1))
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
            if (minus)
                handler_->Int(-(int)i);
            else
                handler_->Uint(i);
        }
    }
}

void XmlReader::ParseString(int tag)
{
    log_trace();
    const char* str;
    size_t length;
    WriteStringStream wsstream(DefaultParseReserve);

    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        str = is_.PutBegin();
        ParseStringToStream(is_);
        is_.Get();
        is_.Put('\0'); // terminate the string
        length = is_.PutEnd() - 1;
        tagSkipFirstChar_ = true;
    }
    else
    {
        // When parsing to a new stream, need to create the local stream to write to.
        ParseStringToStream(wsstream);
        length = wsstream.Length();
        str = wsstream.GetBuffer();
    }
    if ((tag == stringTag) && (GetNextTag() != stringEndTag))
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");

    handler_->String(str,length,copy_);
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
    if (GetNextTag() != nameTag)
        anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");

    const char* str;
    size_t length;
    WriteStringStream wsstream(DefaultParseReserve);

    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        str = is_.PutBegin();
        ParseStringToStream(is_);
        is_.Get();
        is_.Put('\0'); // terminate the string
        length = is_.PutEnd() - 1;
        tagSkipFirstChar_ = true;
    }
    else
    {
        // When parsing to a new stream, need to create the local stream to write to.
        ParseStringToStream(wsstream);
        length = wsstream.Length();
        str = wsstream.GetBuffer();
    }
    log_info("ParseKey: length = " << length);
    if (GetNextTag() != nameEndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
    handler_->Key(str,length,copy_);
}

static const char rawEntity[]  = {  '<',  '>',   '&',   '\'',   '\"', 0 };
static const char* xmlEntity[] = { "lt", "gt", "amp", "apos", "quot", 0 };

void XmlReader::ParseStringToStream(Stream& os)
{
    log_trace();
    while (!is_.Eof())
    {
        char c = is_.Peek();
        if (c == '<') // start of new tag
            return;
        if (c == '&') // Escape
        {
            is_.Get();
            // find the encoded sequence
            int encodedLength = 0;
            char encoded[MaxXmlTagLength];
            bool done = false;
            while (!done)
            {
                if (is_.Eof())
                    return;
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
                        anyrpc_throw(AnyRpcErrorStringEscapeInvalid,
                                "Invalid escape character in string");
                }
            }
            if (encodedLength == 0)
                anyrpc_throw(AnyRpcErrorStringEscapeInvalid,
                        "Invalid escape character in string");
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
                        anyrpc_throw(AnyRpcErrorStringEscapeInvalid,
                                "Invalid escape character in string");
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
            os.Put( is_.Get() );
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
        switch (GetNextTag())
        {
            case memberTag :
            {
                if (memberCount != 0)
                    handler_->MapSeparator();

                ParseKey();
                ParseValue();
                if (GetNextTag() != memberEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
                break;
            }
            case structEndTag :
            {
                handler_->EndMap(memberCount);
                return;

            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
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
    int nextTag = GetNextTag();
    if (nextTag == dataEmptyTag)
    {
        // handle an empty data tag : <array><data/></array>
        handler_->StartArray();
        handler_->EndArray(0);
        if (GetNextTag() != arrayEndTag)
            anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
        return;
    }
    if (nextTag != dataTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");

    handler_->StartArray();

    for (size_t elementCount = 0; ;elementCount++)
    {
        switch (GetNextTag())
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
                if (GetNextTag() != arrayEndTag)
                    anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
                return;
            }
            default :
                anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");
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

    if (GetNextTag() != dateTimeEndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");

    // convert string to time
    struct tm t;
    if ((length != 17) ||
        (sscanf(str, "%4d%2d%2dT%2d:%2d:%2d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6))
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

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
    if (GetNextTag() != base64EndTag)
        anyrpc_throw(AnyRpcErrorTagInvalid, "Parse error with xml tag");

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
