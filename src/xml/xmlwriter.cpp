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
#include "anyrpc/internal/strtod.h"
#include "anyrpc/xml/xmlwriter.h"

#include <limits>

namespace anyrpc
{

void XmlWriter::Null()
{
    log_debug("Null");
    os_.Put("<value><nil/></value>");
}

void XmlWriter::BoolTrue()
{
    log_debug("BoolTrue");
    os_.Put("<value><boolean>1</boolean></value>");
}

void XmlWriter::BoolFalse()
{
    log_debug("BoolFalse");
    os_.Put("<value><boolean>0</boolean></value>");
}

void XmlWriter::Int(int i)
{
    log_debug("Int: " << i);
    os_.Put("<value><i4>");
    os_ << i;
    os_.Put("</i4></value>");
}

void XmlWriter::Uint(unsigned u)
{
    log_debug("Uint: " << u);
    os_.Put("<value><i4>");
    os_ << u;
    os_.Put("</i4></value>");
}

void XmlWriter::Int64(int64_t i64)
{
    log_debug("Int64: " << i64);
    os_.Put("<value><i8>");
    os_ << i64;
    os_.Put("</i8></value>");
}

void XmlWriter::Uint64(uint64_t u64)
{
    log_debug("Uint64: " << u64);
    os_.Put("<value><i8>");
    os_ << u64;
    os_.Put("</i8></value>");
}

void XmlWriter::Double(double d)
{
    log_debug("Double: " << d);
    os_.Put("<value><double>");
    if (precision_ > 0)
    {
        char buffer[100];
        int length = snprintf(buffer, sizeof(buffer), "%.*g", precision_, d);
        os_.Put( buffer, length );
    }
    else
    {
        WriteStringStream ws;
        DoubleToStream(ws, d);
        os_.Put( ws.GetBuffer(), ws.Length() );
    }

    os_.Put("</double></value>");
}

void XmlWriter::DateTime(time_t dt)
{
    char buffer[100];
    struct tm t;
    localtime_r(&dt, &t);
    //gmtime_r(&dt, &t);
    int length = snprintf(buffer, sizeof(buffer), "%4d%02d%02dT%02d:%02d:%02d",
            1900+t.tm_year, 1+t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    os_.Put("<value><dateTime.iso8601>");
    os_.Put( buffer, length );
    os_.Put("</dateTime.iso8601></value>");
    log_debug("DateTime: " << buffer);
}

void XmlWriter::String(const char* str, size_t length, bool copy)
{
    if (length < 10)
        log_debug("String: length=" << length << ", str=" << str);
    else
    {
        char logStr[11];
        memcpy(logStr,str,10);
        logStr[10] = 0;
        log_debug("String: length=" << length << ", str=" << logStr << "...");
    }

    os_.Put("<value>");
    //os_.Put("<string>"); // optional but easier to handle whitespace in strings
    StringData(str, length);
    //os_.Put("</string>");
    os_.Put("</value>");
}

void XmlWriter::StringData(const char* str, size_t length)
{
    static const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    static const char escape[256] = {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
        'i', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u',   0,   0, 'u', 'u',   0, 'u', 'u', // 00
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
          0,   0, '"',   0,   0,   0, '&','\'',   0,   0,   0,   0,   0,   0,   0,   0, // 20
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '<',   0, '>',   0, // 30
        Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                      // 40~FF
#undef Z16
    };

    for (size_t i=0; i<length; i++)
    {
        unsigned char c = static_cast<unsigned>(*str);
        if (escape[c])
        {
            switch (escape[c])
            {
                case '<' : os_.Put("&lt;"); break;
                case '>' : os_.Put("&gt;"); break;
                case '&' : os_.Put("&amp;"); break;
                case '\'': os_.Put("&apos;"); break;
                case '"' : os_.Put("&quot;"); break;
                case 'u' :
                {
                    os_.Put("&#x");
                    os_.Put(hexDigits[(unsigned char)c >> 4]);
                    os_.Put(hexDigits[(unsigned char)c & 0xF]);
                    os_.Put(';');
                    break;
                }
                case 'i' : anyrpc_throw(AnyRpcErrorNullInString, "Null value detection in string");
            }
        }
        else
        {
            os_.Put(c);
        }
        str++;
    }
}

void XmlWriter::Binary(const unsigned char* str, size_t length, bool copy)
{
    log_debug("Binary: length=" << length);
    os_.Put( "<value><base64>" );
    internal::Base64Encode( os_, str, length );
    os_.Put("</base64></value>");
}

void XmlWriter::StartMap()
{
    log_debug("StartMap");
    StartToken("<value><struct>");
}

void XmlWriter::Key(const char* str, size_t length, bool copy)
{
    log_debug("Key: length=" << length << ", str=" << str);
    StartLine();
    StartToken("<member>");
    StartLine();
    os_.Put("<name>");
    StringData(str, length);
    os_.Put("</name>");
    StartLine();
}

void XmlWriter::MapSeparator()
{
    log_debug("MapSeparator");
    EndToken("</member>");
}

void XmlWriter::EndMap(size_t memberCount)
{
    log_debug("EndMap: count=" << memberCount);
    if (memberCount > 0)
    {
        EndToken("</member>");
    }
    EndToken("</struct></value>");
    os_.Flush();
}

void XmlWriter::StartArray(size_t elementCount)
{
    log_debug("StartArray: count=" << elementCount);
    StartToken("<value><array><data>");
    if (elementCount != 0)
        StartLine();
}

void XmlWriter::ArraySeparator()
{
    log_debug("ArraySeparator");
    StartLine();
}

void XmlWriter::EndArray(size_t elementCount)
{
    log_debug("EndArray: count=" << elementCount);
    EndToken("</data></array></value>");
    os_.Flush();
}

void XmlWriter::StartLine()
{
    if (pretty_)
    {
        os_.Put('\n');
        for( int i=0; i<level_; i++)
            os_.Put("\t");
    }
}

void XmlWriter::StartToken(const char* pToken)
{
    os_.Put(pToken);
    if (pretty_)
        level_++;
}

void XmlWriter::EndToken(const char* pToken)
{
    if (pretty_)
    {
        level_--;
        anyrpc_assert(level_ >= 0, AnyRpcErrorPrettyPrintLevel, "Pretty printing level underflow");
        StartLine();
    }
    os_.Put(pToken);
}

void XmlWriter::DoubleToStream(Stream& os, double value)
{
    if (value == 0.0)
    {
        os.Put('0');
        return;
    }
    if (value < 0.0)
    {
        os.Put('-');
        value = -value;
    }

    int power;
    internal::DoubleExtractPower(value, power);
    double precision = std::numeric_limits<double>::epsilon();
    if (power >= 0)
    {
        // Generate digits before the decimal point
        for ( ; power >= 0; power--)
        {
            unsigned digit = LeadDigit(value, precision);
            os.Put('0' + digit);
            value = (value - digit) * 10.0;
            precision *= 10.0;
            if (value < precision)
            {
                // Past the precision of the number - zeros from here
                value = 0;
                for ( ;power > 0; power--)
                    os.Put('0');
            }
        }
        if (value > precision)
        {
            os.Put('.');
            // Output digits until precision reached
            while (value > precision)
            {
                unsigned digit = LeadDigit(value, precision);
                os.Put('0' + digit);
                value = (value - digit) * 10.0;
                precision *= 10.0;
            }
        }
    }
    else
    {
        os.Put("0.");
        // Do the leading zeroes, which eat no precision
        for(int i=0; i<-(power+1); i++)
            os.Put('0');
        // Output digits until precision reached
        while (value > precision)
        {
            unsigned digit = LeadDigit(value, precision);
            os.Put('0' + digit);
            value = (value - digit) * 10.0;
            precision *= 10.0;
        }
    }
}

std::string ToXmlString(Value& value, int precision, bool pretty)
{
    WriteStringStream strStream;
    XmlWriter xmlStrWriter(strStream, pretty);
    if (precision > 0)
        xmlStrWriter.SetScientificPrecision(precision);
    xmlStrWriter << value;
    return strStream.GetString();
}

}
