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
#include "anyrpc/internal/base64.h"
#include "anyrpc/internal/time.h"
#include "anyrpc/json/jsonwriter.h"

namespace anyrpc
{

void JsonWriter::Null()
{
    log_debug("Null");
    os_.Put("null");
}

void JsonWriter::BoolTrue()
{
    log_debug("BoolTrue");
    os_.Put("true");
}

void JsonWriter::BoolFalse()
{
    log_debug("BoolFalse");
    os_.Put("false");
}

void JsonWriter::Int(int i)
{
    log_debug("Int: " << i);
    os_ << i;
}

void JsonWriter::Uint(unsigned u)
{
    log_debug("Uint: " << u);
    os_ << u;
}

void JsonWriter::Int64(int64_t i64)
{
    log_debug("Int64: " << i64);
    os_ << i64;
}

void JsonWriter::Uint64(uint64_t u64)
{
    log_debug("Uint64: " << u64);
    os_ << u64;
}

void JsonWriter::Double(double d)
{
    log_debug("Double: " << d);
    char buffer[100];
    int length;
    if (precision_ > 0)
        length = snprintf(buffer, sizeof(buffer), "%.*g", precision_, d);
    else
        length = snprintf(buffer, sizeof(buffer), "%g", d);
    os_.Put( buffer, length );
}

void JsonWriter::DateTime(time_t dt)
{
    char buffer[100];
    struct tm t;
    localtime_r(&dt, &t);
    //gmtime_r(&dt, &t);
    int length = snprintf(buffer, sizeof(buffer), "[\"%s\",\"%4d%02d%02dT%02d:%02d:%02d\"]",
            ANYRPC_DATETIME_STRING, 1900+t.tm_year, 1+t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    os_.Put( buffer, length );
    log_debug("DateTime: " << buffer);
}

void JsonWriter::String(const char* str, size_t length, bool copy)
{
#if defined(BUILD_WITH_LOG4CPLUS)
    if (length < 10)
        log_debug("String: length=" << length << ", str=" << str);
    else
    {
        char logStr[11];
        memcpy(logStr,str,10);
        logStr[10] = 0;
        log_debug("String: length=" << length << ", str=" << logStr << "...");
    }
#endif
    static const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    static const char escape[256] = {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
          0,   0, '"',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20
        Z16, Z16,                                                                       // 30~4F
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\\',   0,   0,   0, // 50
        Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                                // 60~FF
#undef Z16
    };

    os_.Put('\"');
    for (size_t i=0; i<length; i++)
    {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (escape[c])
        {
            os_.Put('\\');
            os_.Put(escape[c]);
            if (escape[(unsigned char)c] == 'u')
            {
                os_.Put('0');
                os_.Put('0');
                os_.Put(hexDigits[(c >> 4)      ]);
                os_.Put(hexDigits[(c     ) & 0xF]);
            }
        }
        else if ((encoding_ == ASCII) && (c >= 0x80))
        {
            // Unicode escape
            unsigned codepoint;
            DecodeUtf8(str,length,i,codepoint);
            os_.Put("\\u");
            if ((codepoint <= 0xD7FF) || ((codepoint >= 0xE000) && (codepoint <= 0xFFFF)))
            {
                os_.Put(hexDigits[(codepoint >> 12) & 0xF]);
                os_.Put(hexDigits[(codepoint >>  8) & 0xF]);
                os_.Put(hexDigits[(codepoint >>  4) & 0xF]);
                os_.Put(hexDigits[(codepoint      ) & 0xF]);
            }
            else
            {
                anyrpc_assert( (codepoint >= 0x010000) && (codepoint <= 0x10FFFF), AnyRpcErrorUnicodeValue,
                                "Unicode codepoint out of range: " << codepoint);
                unsigned s = codepoint - 0x010000;
                unsigned lead  = (s >> 10)   + 0xD800;
                unsigned trail = (s & 0x3FF) + 0xDC00;
                os_.Put(hexDigits[(lead  >> 12) & 0xF]);
                os_.Put(hexDigits[(lead  >>  8) & 0xF]);
                os_.Put(hexDigits[(lead  >>  4) & 0xF]);
                os_.Put(hexDigits[(lead       ) & 0xF]);
                os_.Put('\\');
                os_.Put('u');
                os_.Put(hexDigits[(trail >> 12) & 0xF]);
                os_.Put(hexDigits[(trail >>  8) & 0xF]);
                os_.Put(hexDigits[(trail >>  4) & 0xF]);
                os_.Put(hexDigits[(trail      ) & 0xF]);
            }
        }
        else
        {
            os_.Put(c);
        }
    }
    os_.Put('\"');
}

void JsonWriter::DecodeUtf8(const char* str, size_t length, size_t &pos, unsigned &codepoint)
{
    unsigned char c = static_cast<unsigned char>(str[pos]);
    if (c < 0xC0)
        anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, c=" << c);
    size_t dataRemaining = length - pos;
    if (c < 0xE0)
    {
        // two byte sequence
        if (dataRemaining < 2)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, not enough data");
        unsigned char c2 = static_cast<unsigned char>(str[++pos]);
        if ((c2 & 0xc0) != 0x80)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, c2=" << c2);
        codepoint = ((c & 0x1F) << 6) + (c2 & 0x3F);
        log_debug("Decode 4: " << (int)c << ", " << (int)c2);
        if (codepoint < 0x80)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, codepoint=" << codepoint);
        return;
    }
    if (c < 0xF0)
    {
        // three byte sequence
        if (dataRemaining < 3)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, not enough data");
        unsigned char c2 = static_cast<unsigned char>(str[++pos]);
        unsigned char c3 = static_cast<unsigned char>(str[++pos]);
        if (((c2 & 0xc0) != 0x80) || ((c3 & 0xc0) != 0x80))
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, c2=" << c2 << ", c3=" << c3);
        codepoint = ((c & 0x0F) << 12) + ((c2 & 0x3F) << 6) + (c3 & 0x3F);
        log_debug("Decode 3: " << (int)c << ", " << (int)c2 << ", " << (int)c3);
        if (codepoint < 0x800)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, codepoint=" << codepoint);
        return;
    }
    if (c < 0xF8)
    {
        // four byte sequence
        if (dataRemaining < 4)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, not enough data");
        unsigned char c2 = static_cast<unsigned char>(str[++pos]);
        unsigned char c3 = static_cast<unsigned char>(str[++pos]);
        unsigned char c4 = static_cast<unsigned char>(str[++pos]);
        if (((c2 & 0xc0) != 0x80) || ((c3 & 0xc0) != 0x80) || ((c4 & 0xc0) != 0x80))
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, c2=" << c2 << ", c3=" << c3 << ", c4=" << c4);
        codepoint = ((c & 0x07) << 18) + ((c2 & 0x3F) << 12) + ((c3 & 0x3F) << 6) + (c4 & 0x3F);
        log_debug("Decode 4: " << (int)c << ", " << (int)c2 << ", " << (int)c3 << ", " << (int)c4);
        if (codepoint < 0x10000)
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, codepoint=" << codepoint);
        return;
    }
    anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence, c=" << c);
}

void JsonWriter::Binary(const unsigned char* str, size_t length, bool copy)
{
    log_debug("Binary: length=" << length);
    char buffer[100];
    int bufLen = snprintf(buffer, sizeof(buffer), "[\"%s\",\"", ANYRPC_BASE64_STRING );
    os_.Put( buffer, bufLen );
    internal::Base64Encode( os_, str, length );
    os_.Put("\"]");
}

void JsonWriter::StartMap()
{
    log_debug("StartMap");
    NewLine();
    os_.Put('{');
    IncLevel();
    NewLine();
}

void JsonWriter::Key(const char* str, size_t length, bool copy)
{
    log_debug("Key: length=" << length << ", str=" << str);
    String(str, length, copy);
    os_.Put(':');
}

void JsonWriter::MapSeparator()
{
    log_debug("MapSeparator");
    os_.Put(',');
    NewLine();
}

void JsonWriter::EndMap(size_t memberCount)
{
    log_debug("EndMap: count=" << memberCount);
    DecLevel();
    NewLine();
    os_.Put('}');
    os_.Flush();
}

void JsonWriter::StartArray(size_t elementCount)
{
    log_debug("StartArray");
    NewLine();
    os_.Put('[');
    IncLevel();
    NewLine();
}

void JsonWriter::ArraySeparator()
{
    log_debug("ArraySeparator");
    os_.Put(',');
    NewLine();
}

void JsonWriter::EndArray(size_t elementCount)
{
    log_debug("EndArray: count=" << elementCount);
    DecLevel();
    NewLine();
    os_.Put(']');
    os_.Flush();
}

void JsonWriter::NewLine()
{
    if (pretty_)
    {
        os_.Put('\n');
        for( int i=0; i<level_; i++)
            os_.Put("\t");
    }
}

void JsonWriter::IncLevel()
{
    if (pretty_)
        level_++;
}

void JsonWriter::DecLevel()
{
    if (pretty_)
    {
        level_--;
        anyrpc_assert(level_ >= 0, AnyRpcErrorPrettyPrintLevel, "Pretty printing level underflow");
    }
}

std::string ToJsonString(Value& value, EncodingEnum encoding, unsigned precision, bool pretty)
{
    WriteStringStream strStream;
    JsonWriter jsonStrWriter(strStream, encoding, precision, pretty);
    jsonStrWriter << value;
    return strStream.GetString();
}

}
