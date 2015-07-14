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
#include "anyrpc/json/jsonreader.h"
#include "anyrpc/internal/strtod.h"

#include <cctype>

namespace anyrpc
{

void JsonReader::ParseStream(Handler& handler)
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

void JsonReader::ParseStream()
{
    log_trace();
    SkipWhiteSpace();

    if (!is_.Eof())
    {
        ParseValue();
    }
}

void JsonReader::ParseValue()
{
    log_trace();
    // Look at the next character to determine the type of parsing to perform
    switch (is_.Peek()) {
        case 'n': ParseNull(); break;
        case 't': ParseTrue(); break;
        case 'f': ParseFalse(); break;
        case '"': ParseString(); break;
        case '{': ParseMap(); break;
        case '[': ParseArray(); break;
        default : ParseNumber();
    }
}

void JsonReader::ParseNull()
{
    log_trace();
    if ((is_.Get() != 'n') ||
        (is_.Get() != 'u') ||
        (is_.Get() != 'l') ||
        (is_.Get() != 'l'))
        anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");

    handler_->Null();
}

void JsonReader::ParseTrue()
{
    log_trace();
    if ((is_.Get() != 't') ||
        (is_.Get() != 'r') ||
        (is_.Get() != 'u') ||
        (is_.Get() != 'e'))
        anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");

    handler_->BoolTrue();
}

void JsonReader::ParseFalse()
{
    log_trace();
    if ((is_.Get() != 'f') ||
        (is_.Get() != 'a') ||
        (is_.Get() != 'l') ||
        (is_.Get() != 's') ||
        (is_.Get() != 'e'))
        anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");

    handler_->BoolFalse();
}

void JsonReader::ParseString()
{
    log_trace();
    // This should only be called when a quote has already been determined to be the next character
    log_assert(is_.Peek() == '\"', "Expected \" but found " << is_.Peek());
    is_.Get();

    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        char* str = is_.PutBegin();
        ParseStringToStream(is_);
        size_t length = is_.PutEnd() - 1;
        handler_->String(str,length,copy_);
    }
    else
    {
        // When parsing to a new stream, need to create the local stream to write to.
        // WriteStringStream must stay in scope for the handler_->String call
        WriteStringStream wsstream(DefaultParseReserve);
        ParseStringToStream(wsstream);
        size_t length = wsstream.Length() - 1;
        const char* str = wsstream.GetBuffer();
        handler_->String(str,length);
    }
}

void JsonReader::ParseKey()
{
    log_trace();
    // This should only be called when a quote has already been determined to be the next character
    log_assert(is_.Peek() == '\"', "Expected \" but found " << is_.Peek());
    is_.Get();

    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        char* str = is_.PutBegin();
        ParseStringToStream(is_);
        size_t length = is_.PutEnd() - 1;
        handler_->Key(str,length,copy_);
    }
    else
    {
        // When parsing to a new stream, need to create the local stream to write to.
        // WriteStringStream must stay in scope for the handler_->Key call
        WriteStringStream wsstream(DefaultParseReserve);
        ParseStringToStream(wsstream);
        size_t length = wsstream.Length() - 1;
        const char* str = wsstream.GetBuffer();
        handler_->Key(str,length);
    }
}

// This function is adapted from RapidJason (https://github.com/miloyip/rapidjson)
void JsonReader::ParseStringToStream(Stream& os)
{
    log_trace();
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    static const char escape[256] = {
        Z16, Z16, 0, 0,'\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'/',
        Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
        0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
        0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16
    };
#undef Z16

    while (true)
    {
        char c = is_.Peek();
        if (c == '\\') // Escape
        {
            is_.Get();
            unsigned char e = is_.Get();
            if (escape[e])
            {
                os.Put(escape[e]);
            }
            else if (e == 'u')    // Unicode
            {
                unsigned codepoint = ParseHex4();
                if ((codepoint >= 0xD800) && (codepoint <= 0xDBff))
                {
                    // Handle as UTF-16 surrogate pair
                    if ((is_.Get() != '\\') || (is_.Get() != 'u'))
                        anyrpc_throw(AnyRpcErrorStringUnicodeSurrogateInvalid,
                                "The surrogate pair in string is invalid");
                    unsigned codepoint2 = ParseHex4();
                    if ((codepoint2 < 0xDC00) || (codepoint2 > 0xDFFF))
                        anyrpc_throw(AnyRpcErrorStringUnicodeSurrogateInvalid,
                                "The surrogate pair in string is invalid");
                    codepoint = (((codepoint - 0xD800) << 10) | (codepoint2 - 0xDC00)) + 0x10000;
                }
                EncodeUtf8(os,codepoint);
            }
            else
                anyrpc_throw(AnyRpcErrorStringEscapeInvalid,
                        "Invalid escape character in string");
        }
        else if (c == '"')     // Closing double quote
        {
            is_.Get();
            os.Put('\0');   // null-terminate the string
            return;
        }
        else if (c == '\0')
            anyrpc_throw(AnyRpcErrorStringMissingQuotationMark,
                    "Missing a closing quotation mark in string");
        else if ((unsigned)c < 0x20) // RFC 4627: unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
            anyrpc_throw(AnyRpcErrorStringEscapeInvalid,
                    "Invalid escape character in string");
        else
            os.Put( is_.Get() );
    }

}

// This function is adapted from RapidJason (https://github.com/miloyip/rapidjson)
unsigned JsonReader::ParseHex4()
{
    unsigned codepoint = 0;
    for (int i=0; i<4; i++)
    {
        char c = is_.Get();
        codepoint <<= 4;
        if ((c >= '0') && (c <= '9'))
            codepoint += (c - '0');
        else if ((c >= 'A') && (c <= 'F'))
            codepoint += (c - 'A' + 10);
        else if ((c >= 'a') && (c <= 'f'))
            codepoint += (c - 'a' + 10);
        else
            anyrpc_throw(AnyRpcErrorStringUnicodeEscapeInvalid,
                    "Incorrect digit after escape in string");
    }
    return codepoint;
}

// This function is adapted from RapidJason (https://github.com/miloyip/rapidjson)
void JsonReader::EncodeUtf8(Stream& os, unsigned codepoint)
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

void JsonReader::ParseMap()
{
    log_trace();
    log_assert(is_.Peek() == '{', "Expected { but found " << is_.Peek());
    is_.Get();  // Skip '{'

    handler_->StartMap();

    SkipWhiteSpace();

    if (is_.Peek() == '}')
    {
        is_.Get();
        handler_->EndMap(0);  // empty object
        return;
    }

    for (size_t memberCount = 0;;)
    {
        if (is_.Peek() != '\"')
            anyrpc_throw(AnyRpcErrorObjectMissName, "Missing a name for object member");

        ParseKey();
        SkipWhiteSpace();

        if (is_.Get() != ':')
            anyrpc_throw(AnyRpcErrorObjectMissColon, "Missing a colon after a name of object member");

        SkipWhiteSpace();
        ParseValue();
        SkipWhiteSpace();

        ++memberCount;

        switch (is_.Get())
        {
            case ',':
                SkipWhiteSpace();
                handler_->MapSeparator();
                break;
            case '}':
                handler_->EndMap(memberCount);
                return;
            default:
                anyrpc_throw(AnyRpcErrorObjectMissCommaOrCurlyBracket,
                        "Missing a comma or '}' after an object member");
        }
    }
}

void JsonReader::ParseArray()
{
    log_trace();
    log_assert(is_.Peek() == '[', "Expected [ but found " << is_.Peek());
    is_.Get();  // Skip '['

    handler_->StartArray();

    SkipWhiteSpace();

    if (is_.Peek() == ']')
    {
        is_.Get();
        handler_->EndArray(0); // empty array
        return;
    }

    for (size_t elementCount = 0;;)
    {
        ParseValue();

        ++elementCount;
        SkipWhiteSpace();

        switch (is_.Get())
        {
            case ',':
                SkipWhiteSpace();
                handler_->ArraySeparator();
                break;
            case ']':
                handler_->EndArray(elementCount);
                return;
            default:
                anyrpc_throw(AnyRpcErrorArrayMissCommaOrSquareBracket,
                        "Missing a comma or ']' after an array element");
        }
    }
}

// This function is adapted from RapidJason (https://github.com/miloyip/rapidjson)
void JsonReader::ParseNumber()
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

        handler_->Double(minus ? -d : d);
    }
    else {
        if (use64bit)
        {
            if (minus)
                handler_->Int64(-(int64_t)i64);
            else
                handler_->Uint64(i64);
        }
        else
        {
            if (minus)
                handler_->Int(-(int)i);
            else
                handler_->Uint(i);
        }
    }
}

}
