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
#include "anyrpc/messagepack/messagepackwriter.h"
#include "anyrpc/messagepack/sysdep.h"
#include "anyrpc/messagepack/messagepackformat.h"
#include "anyrpc/internal/time.h"

namespace anyrpc
{

#if defined(__LITTLE_ENDIAN__)
template <typename T>
inline char take8_8(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[0]);
}
template <typename T>
inline char take8_16(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[0]);
}
template <typename T>
inline char take8_32(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[0]);
}
template <typename T>
inline char take8_64(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[0]);
}

#elif defined(__BIG_ENDIAN__)

template <typename T>
inline char take8_8(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[0]);
}
template <typename T>
inline char take8_16(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[1]);
}
template <typename T>
inline char take8_32(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[3]);
}
template <typename T>
inline char take8_64(T d) {
    return static_cast<char>(reinterpret_cast<uint8_t*>(&d)[7]);
}

#endif

void MessagePackWriter::Null()
{
    log_debug("Null");
    os_.Put(MessagePackNil);
}

void MessagePackWriter::BoolTrue()
{
    log_debug("BoolTrue");
    os_.Put(MessagePackTrue);
}

void MessagePackWriter::BoolFalse()
{
    log_debug("BoolFalse");
    os_.Put(MessagePackFalse);
}

void MessagePackWriter::Int(int i)
{
    log_debug("Int: " << i);
    if (i < -(1<<5))
    {
        if (i < -(1<<15))
        {
            // use signed 32 representation
            char buf[4];
            os_.Put(MessagePackInt32);
            _msgpack_store32(buf, static_cast<int32_t>(i));
            os_.Put(buf,4);
        }
        else if (i < -(1<<7))
        {
            // use signed 16 representation
            char buf[2];
            os_.Put(MessagePackInt16);
            _msgpack_store16(buf, static_cast<int16_t>(i));
            os_.Put(buf,2);
        }
        else
        {
            // use signed 8 representation
            os_.Put(MessagePackInt8);
            os_.Put(take8_32(i));
        }
    }
    else if (i < (1<<7))
    {
        // fixnum - could be in range -32 to 127
        os_.Put(take8_32(i));
    }
    else
    {
        // reuse the code to write a positive integer
        WriteUint(static_cast<unsigned>(i));
    }
}

void MessagePackWriter::Uint(unsigned u)
{
    log_debug("Uint: " << u);
    WriteUint(u);
}

void MessagePackWriter::Int64(int64_t i64)
{
    log_debug("Int64: " << i64);
    if (i64 < -(1LL << 5))
    {
        if (i64 < -(1LL << 15))
        {
            if (i64 < -(1LL << 31))
            {
                // use signed 64 representation
                char buf[8];
                os_.Put(MessagePackInt64);
                _msgpack_store64(buf, i64);
                os_.Put(buf, 8);
            }
            else
            {
                // use signed 32 representation
                char buf[4];
                os_.Put(MessagePackInt32);
                _msgpack_store32(buf, static_cast<int32_t>(i64));
                os_.Put(buf, 4);
            }
        }
        else
        {
            if (i64 < -(1 << 7))
            {
                // use signed 16 representation
                char buf[2];
                os_.Put(MessagePackInt16);
                _msgpack_store16(buf, static_cast<int16_t>(i64));
                os_.Put(buf, 2);
            }
            else
            {
                // use signed 8 representation
                os_.Put(MessagePackInt8);
                os_.Put(take8_64(i64));
            }
        }
    }
    else if (i64 < (1 << 7))
    {
        // fixnum - could be in range -32 to 127
        os_.Put(take8_64(i64));
    }
    else
    {
        // reuse the code to write a positive integer
        WriteUint64(static_cast<uint64_t>(i64));
    }
}

void MessagePackWriter::Uint64(uint64_t u64)
{
    log_debug("Uint64: " << u64);
    WriteUint64(u64);
}

void MessagePackWriter::Float(float f)
{
    log_debug("Float: " << f);
    union { float f; uint32_t i; } mem;
    mem.f = f;
    char buf[4];
    os_.Put(MessagePackFloat32);
    _msgpack_store32(buf, mem.i);
    os_.Put(buf,4);
}

void MessagePackWriter::Double(double d)
{
    log_debug("Double: " << d);
    union { double f; uint64_t i; } mem;
    mem.f = d;
    char buf[8];
    os_.Put(MessagePackFloat64);
#if defined(__arm__) && !(__ARM_EABI__) // arm-oabi
    // https://github.com/msgpack/msgpack-perl/pull/1
    mem.i = (mem.i & 0xFFFFFFFFUL) << 32UL | (mem.i >> 32UL);
#endif
    _msgpack_store64(buf, mem.i);
    os_.Put(buf,8);
}

void MessagePackWriter::DateTime(time_t dt)
{
    log_debug("DateTime: " << dt);
    // use array representation - two elements
    StartArray(2);
    // indicate AnyRpc DataTime string
    String(ANYRPC_DATETIME_STRING, strlen(ANYRPC_DATETIME_STRING));

    // create a string representation of the date - similar to xmlrpc handling
    char buffer[100];
    struct tm t;
    localtime_r(&dt, &t);
    //gmtime_r(&dt, &t);
    int length = snprintf(buffer, sizeof(buffer), "%4d%02d%02dT%02d:%02d:%02d",
             1900+t.tm_year, 1+t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    String( buffer, length );

    // end of the array
    EndArray(2);
}

void MessagePackWriter::String(const char* str, size_t length, bool copy)
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

    // output the string type identifier with the length
    if (length < 32)
    {
        // use fixed string format
        os_.Put(MessagePackFixStr | static_cast<uint8_t>(length));
    }
    else if (length < 256)
    {
        // use str 8 representation
        os_.Put(MessagePackStr8);
        os_.Put(static_cast<uint8_t>(length));
    }
    else if (length < 65536)
    {
        // use str 16 representation
        char buf[2];
        os_.Put(MessagePackStr16);
        _msgpack_store16(buf, static_cast<uint16_t>(length));
        os_.Put(buf, 2);
    }
    else
    {
        // use str 32 representation
        char buf[4];
        os_.Put(MessagePackStr32);
        _msgpack_store32(buf, static_cast<uint32_t>(length));
        os_.Put(buf, 4);
    }
    // output the string characters
    os_.Put(str, length);
}

void MessagePackWriter::Binary(const unsigned char* str, size_t length, bool copy)
{
    log_debug("Binary: length=" << length);
    // output the binary type identifier with the length
    if (length < 256)
    {
        // use bin 8 representation
        os_.Put(MessagePackBin8);
        os_.Put(static_cast<uint8_t>(length));
    }
    else if (length < 65536)
    {
        // use bin 16 representation
        char buf[2];
        os_.Put(MessagePackBin16);
        _msgpack_store16(buf, static_cast<uint16_t>(length));
        os_.Put(buf, 2);
    }
    else
    {
        // use bin 32 representation
        char buf[4];
        os_.Put(MessagePackBin32);
        _msgpack_store32(buf, static_cast<uint32_t>(length));
        os_.Put(buf, 4);
    }
    // output the binary data block
    os_.Put((const char*)str, length);
}

void MessagePackWriter::StartMap(size_t memberCount)
{
    log_debug("StartMap: count=" << memberCount);
    if (memberCount < 16)
    {
        // use fixmap representation - up to 15 elements
        os_.Put(MessagePackFixMap | memberCount);
    }
    else if (memberCount < 65536)
    {
        // use map 16 representation
        char buf[2];
        os_.Put(MessagePackMap16);
        _msgpack_store16(buf, static_cast<uint16_t>(memberCount));
        os_.Put(buf, 2);
    }
    else
    {
        // use map32 representation
        char buf[4];
        os_.Put(MessagePackMap32);
        _msgpack_store32(buf, static_cast<uint32_t>(memberCount));
        os_.Put(buf, 4);
    }
}

void MessagePackWriter::Key(const char* str, size_t length, bool copy)
{
    // Write the key as a string element
    log_debug("Key: length=" << length << ", str=" << str);
    String(str, length, copy);
}

void MessagePackWriter::EndMap(size_t memberCount)
{
    log_debug("EndMap: count=" << memberCount);
    os_.Flush();
}

void MessagePackWriter::StartArray(size_t elementCount)
{
    log_debug("StartArray: count=" << elementCount);
    if (elementCount < 16)
    {
        // use fixarray representation - up to 15 elements
        os_.Put(MessagePackFixArray | elementCount);
    }
    else if (elementCount < 65536)
    {
        // use array 16 representation
        char buf[2];
        os_.Put(MessagePackArray16);
        _msgpack_store16(buf, static_cast<uint16_t>(elementCount));
        os_.Put(buf, 2);
    }
    else
    {
        // use array 32 representation
        char buf[4];
        os_.Put(MessagePackArray32);
        _msgpack_store32(buf, static_cast<uint32_t>(elementCount));
        os_.Put(buf, 4);
    }
}

void MessagePackWriter::EndArray(size_t elementCount)
{
    log_debug("EndArray: count=" << elementCount);
    os_.Flush();
}

void MessagePackWriter::WriteUint(unsigned u)
{
    if (u < (1 << 8))
    {
        if (u < (1 << 7))
        {
            // use fixnum representation - could be in range -32 to 127
            os_.Put(take8_32(u));
        }
        else
        {
            // use unsigned 8 representation
            os_.Put(MessagePackUint8);
            os_.Put(take8_32(u));
        }
    }
    else
    {
        if (u < (1 << 16))
        {
            // use unsigned 16 representation
            char buf[2];
            os_.Put(MessagePackUint16);
            _msgpack_store16(buf, static_cast<uint16_t>(u));
            os_.Put(buf, 2);
        }
        else
        {
            // unsigned 32 representation
            char buf[4];
            os_.Put(MessagePackUint32);
            _msgpack_store32(buf, static_cast<uint32_t>(u));
            os_.Put(buf, 4);
        }
    }
}

void MessagePackWriter::WriteUint64(uint64_t u64)
{
    if (u64 < (1ULL << 8))
    {
        if (u64 < (1ULL << 7))
        {
            // use fixnum representation - could be in range -32 to 127
            os_.Put(take8_64(u64));
        }
        else
        {
            // use unsigned 8 representation
            os_.Put(MessagePackUint8);
            os_.Put(take8_64(u64));
        }
    }
    else
    {
        if (u64 < (1ULL << 16))
        {
            // use unsigned 16 representation
            char buf[2];
            os_.Put(MessagePackUint16);
            _msgpack_store16(buf, static_cast<uint16_t>(u64));
            os_.Put(buf, 2);
        }
        else if (u64 < (1ULL << 32))
        {
            // use unsigned 32 representation
            char buf[4];
            os_.Put(MessagePackUint32);
            _msgpack_store32(buf, static_cast<uint32_t>(u64));
            os_.Put(buf, 4);
        }
        else
        {
            // use unsigned 64 representation
            char buf[8];
            os_.Put(MessagePackUint64);
            _msgpack_store64(buf, u64);
            os_.Put(buf, 8);
        }
    }
}

}
