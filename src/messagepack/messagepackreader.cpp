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
#include "anyrpc/messagepack/messagepackreader.h"
#include "anyrpc/messagepack/sysdep.h"
#include "anyrpc/messagepack/messagepackformat.h"

namespace anyrpc
{

void MessagePackReader::ParseStream(Handler& handler)
{
    log_time(INFO);

    // setup for stream parsing
    handler_ = &handler;
    parseError_.Clear();
    ResetToken();

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

#define ParsePFI4   &MessagePackReader::ParsePositiveFixInt, &MessagePackReader::ParsePositiveFixInt, &MessagePackReader::ParsePositiveFixInt, &MessagePackReader::ParsePositiveFixInt
#define ParsePFI16  ParsePFI4, ParsePFI4, ParsePFI4, ParsePFI4
#define ParseFM4    &MessagePackReader::ParseFixMap,         &MessagePackReader::ParseFixMap,         &MessagePackReader::ParseFixMap,         &MessagePackReader::ParseFixMap
#define ParseFA4    &MessagePackReader::ParseFixArray,       &MessagePackReader::ParseFixArray,       &MessagePackReader::ParseFixArray,       &MessagePackReader::ParseFixArray
#define ParseFS4    &MessagePackReader::ParseFixStr,         &MessagePackReader::ParseFixStr,         &MessagePackReader::ParseFixStr,         &MessagePackReader::ParseFixStr
#define ParseNFI4   &MessagePackReader::ParseNegativeFixInt, &MessagePackReader::ParseNegativeFixInt, &MessagePackReader::ParseNegativeFixInt ,&MessagePackReader::ParseNegativeFixInt

MessagePackReader::Function MessagePackReader::ParseLookup[256] = {
        ParsePFI16, ParsePFI16, ParsePFI16, ParsePFI16,         // 0x00 - 0x3f
        ParsePFI16, ParsePFI16, ParsePFI16, ParsePFI16,         // 0x40 - 0x9f
        ParseFM4, ParseFM4, ParseFM4, ParseFM4,                 // 0x80 - 0x8f
        ParseFA4, ParseFA4, ParseFA4, ParseFA4,                 // 0x90 - 0x9f
        ParseFS4, ParseFS4, ParseFS4, ParseFS4,                 // 0xa0 - 0xaf
        ParseFS4, ParseFS4, ParseFS4, ParseFS4,                 // 0xb0 - 0xbf
        &MessagePackReader::ParseNull,                          // 0xc0
        &MessagePackReader::ParseIllegal,                       // 0xc1
        &MessagePackReader::ParseFalse,                         // 0xc2
        &MessagePackReader::ParseTrue,                          // 0xc3
        &MessagePackReader::ParseBin8,                          // 0xc4
        &MessagePackReader::ParseBin16,                         // 0xc5
        &MessagePackReader::ParseBin32,                         // 0xc6
        &MessagePackReader::ParseExt8,                          // 0xc7
        &MessagePackReader::ParseExt16,                         // 0xc8
        &MessagePackReader::ParseExt32,                         // 0xc9
        &MessagePackReader::ParseFloat32,                       // 0xca
        &MessagePackReader::ParseFloat64,                       // 0xcb
        &MessagePackReader::ParseUint8,                         // 0xcc
        &MessagePackReader::ParseUint16,                        // 0xcd
        &MessagePackReader::ParseUint32,                        // 0xce
        &MessagePackReader::ParseUint64,                        // 0xcf
        &MessagePackReader::ParseInt8,                          // 0xd0
        &MessagePackReader::ParseInt16,                         // 0xd1
        &MessagePackReader::ParseInt32,                         // 0xd2
        &MessagePackReader::ParseInt64,                         // 0xd3
        &MessagePackReader::ParseFixExt1,                       // 0xd4
        &MessagePackReader::ParseFixExt2,                       // 0xd5
        &MessagePackReader::ParseFixExt4,                       // 0xd6
        &MessagePackReader::ParseFixExt8,                       // 0xd7
        &MessagePackReader::ParseFixExt16,                      // 0xd8
        &MessagePackReader::ParseStr8,                          // 0xd9
        &MessagePackReader::ParseStr16,                         // 0xda
        &MessagePackReader::ParseStr32,                         // 0xdb
        &MessagePackReader::ParseArray16,                       // 0xdc
        &MessagePackReader::ParseArray32,                       // 0xdd
        &MessagePackReader::ParseMap16,                         // 0xde
        &MessagePackReader::ParseMap32,                         // 0xdf
        ParseNFI4, ParseNFI4, ParseNFI4, ParseNFI4,             // 0xe0 - 0xef
        ParseNFI4, ParseNFI4, ParseNFI4, ParseNFI4              // 0xf0 - 0xff
};

void MessagePackReader::ParseStream()
{
    log_trace();
    if ((token_ == 0) && is_.Eof())
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    else
    {
        GetToken();
        log_debug("token=" << (int)token_);

        // look up the proper function based on the token and call
        (this->*ParseLookup[token_])();
    }
}

void MessagePackReader::ParseKey()
{
    log_trace();
    if (is_.Eof())
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

    GetToken();
    size_t length = 0;

    // only allow string keys although the MessagePack spec allows other types
    if ((token_ >= MessagePackFixStr) && (token_ < MessagePackNil))
        length = token_ & 0x1f;
    else if (token_ == MessagePackStr8)
    {
        if (is_.Eof())
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        length = static_cast<unsigned char>(is_.Get());
    }
    else
        anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value");

    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        char* buffer = is_.PutBegin();
        if (length != is_.Skip(length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

        GetClearToken();  // get the next token and clear the source, i.e. terminate the string
        handler_->Key(buffer,length,copy_);
        is_.PutEnd();
    }
    else if (length > 0)
    {
        // Key strings can only be up to 256 characters
        char str[257];
        if (is_.Read(str,length) != length)
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

        str[length] = 0;
        handler_->Key(str,length);
        ResetToken();
    }
}
void MessagePackReader::ParseNull()
{
    log_trace();
    handler_->Null();
    ResetToken();
}

void MessagePackReader::ParseFalse()
{
    log_trace();
    handler_->BoolFalse();
    ResetToken();
}

void MessagePackReader::ParseTrue()
{
    log_trace();
    handler_->BoolTrue();
    ResetToken();
}

void MessagePackReader::ParsePositiveFixInt()
{
    log_trace();
    handler_->Uint( static_cast<unsigned>(token_) );
    ResetToken();
}

void MessagePackReader::ParseNegativeFixInt()
{
    log_trace();
    handler_->Int( static_cast<signed char>(token_) );
    ResetToken();
}

void MessagePackReader::ParseUint8()
{
    log_trace();
    if (is_.Eof())
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    handler_->Uint( static_cast<unsigned char>(is_.Get()) );
    ResetToken();
}

void MessagePackReader::ParseUint16()
{
    log_trace();
    unsigned short value;
    if (is_.Read( reinterpret_cast<char*>(&value), 2 ) != 2)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    value = _msgpack_be16( value );
    handler_->Uint( static_cast<unsigned short>( value ) );
    ResetToken();
}

void MessagePackReader::ParseUint32()
{
    log_trace();
    unsigned value;
    if (is_.Read( reinterpret_cast<char*>(&value), 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    value = _msgpack_be32( value );
    handler_->Uint( value );
    ResetToken();
}

void MessagePackReader::ParseUint64()
{
    log_trace();
    uint64_t value;
    if (is_.Read( reinterpret_cast<char*>(&value), 8 ) != 8)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    value = _msgpack_be64( value );
    handler_->Uint64( value );
    ResetToken();
}

void MessagePackReader::ParseInt8()
{
    log_trace();
    if (is_.Eof())
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    handler_->Int( static_cast<signed char>(is_.Get()) );
    ResetToken();
}

void MessagePackReader::ParseInt16()
{
    log_trace();
    short value;
    if (is_.Read( reinterpret_cast<char*>(&value), 2 ) != 2)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    value = _msgpack_be16( value );
    handler_->Int( static_cast<short>( value ) );
    ResetToken();
}

void MessagePackReader::ParseInt32()
{
    log_trace();
    int value;
    if (is_.Read( reinterpret_cast<char*>(&value), 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    value = _msgpack_be32( value );
    handler_->Int( value );
    ResetToken();
}

void MessagePackReader::ParseInt64()
{
    log_trace();
    int64_t value;
    if (is_.Read( reinterpret_cast<char*>(&value), 8 ) != 8)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    value = _msgpack_be64( value );
    handler_->Int64( value );
    ResetToken();
}

void  MessagePackReader::ParseFloat32()
{
    log_trace();
    union {
        float f;
        int i;
        char str[4];
    } value;

    if (is_.Read( value.str, 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

    value.i = _msgpack_be32(value.i);
    handler_->Float( value.f );
    ResetToken();
}

void MessagePackReader::ParseFloat64()
{
    log_trace();
    union {
        double d;
        int64_t i64;
        char str[8];
    } value;

    if (is_.Read( value.str, 8 ) != 8)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");

    value.i64 = _msgpack_be64(value.i64);
    handler_->Double( value.d );
    ResetToken();
}

void MessagePackReader::ParseFixStr()
{
    log_trace();
    ParseStr(token_ & 0x1f);
}

void MessagePackReader::ParseStr8()
{
    log_trace();
    ParseStr( static_cast<unsigned char>(is_.Get()) );
}

void MessagePackReader::ParseStr16()
{
    log_trace();
    unsigned short length;
    if (is_.Read( reinterpret_cast<char*>(&length), 2 ) != 2)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be16( length );
    ParseStr( length );
}

void MessagePackReader::ParseStr32()
{
    log_trace();
    unsigned length;
    if (is_.Read( reinterpret_cast<char*>(&length), 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be32( length );
    ParseStr( length );
}

void MessagePackReader::ParseStr(size_t length)
{
    log_debug("MessagePackReader::ParseStr: length=" << length);
    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        char* buffer = is_.PutBegin();
        if (length != is_.Skip(length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        // Value expects all non-copied strings to be null terminated.
        // If you are already at the end of file, then you can't add the null terminator
        bool copy = copy_ || is_.Eof();
        GetClearToken();  // get the next token and clear the source, i.e. terminate the string
        handler_->String(buffer,length,copy);
        is_.PutEnd();
    }
    else if (length <= 256)
    {
        // Read short strings (probably the majority) onto the stack
        char buffer[257];
        if (length != is_.Read(buffer, length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        buffer[length] = 0;
        handler_->String(buffer,length);
        ResetToken();
    }
    else
    {
        // Long strings need an allocated buffer
        // It would be more efficient if the ownership of this buffer could be transferred to the handler
        char *buffer = static_cast<char*>(malloc(length+1));
        if (!buffer)
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        if (length != is_.Read(buffer, length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        buffer[length] = 0;
        handler_->String(buffer,length);
        ResetToken();
        free(buffer);
    }
}

void MessagePackReader::ParseBin8()
{
    log_trace();
    if (is_.Eof())
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    ParseBin( static_cast<unsigned char>(is_.Get()) );
}

void MessagePackReader::ParseBin16()
{
    log_trace();
    unsigned short length;
    if (is_.Read( reinterpret_cast<char*>(&length), 2 ) != 2)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be16( length );
    ParseBin( length );
}

void MessagePackReader::ParseBin32()
{
    log_trace();
    unsigned length;
    if (is_.Read( reinterpret_cast<char*>(&length), 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be32( length );
    ParseBin( length );
}

void MessagePackReader::ParseBin(size_t length)
{
    log_debug("MessagePackReader::ParseBin, length=" << length);
    if (inSitu_)
    {
        // When parsing in place, mark the start position and continue with the decoding
        char* buffer = is_.PutBegin();
        if (length != is_.Skip(length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        handler_->Binary((unsigned char*)buffer,length,copy_);  // binary data is not null terminated so it can be referenced
        ResetToken();
        is_.PutEnd();
    }
    else if (length <= 256)
    {
        // Read short binary data onto the stack
        char buffer[256];
        if (length != is_.Read(buffer, length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        handler_->Binary((unsigned char*)buffer,length);
        ResetToken();
    }
    else
    {
        // Long binary data needs an allocated buffer
        // It would be more efficient if the ownership of this buffer could be transferred to the handler
        char *buffer = static_cast<char*>(malloc(length));
        if (!buffer)
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        if (length != is_.Read(buffer, length))
            anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
        handler_->Binary((unsigned char*)buffer,length);
        ResetToken();
        free(buffer);
    }
}

void MessagePackReader::ParseArray16()
{
    log_trace();
    unsigned short length;
    if (is_.Read( reinterpret_cast<char*>(&length), 2 ) != 2)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be16( length );  // compiling for mingw had problem when this was combined with the next line
    ParseArray(length);
}

void MessagePackReader::ParseArray32()
{
    log_trace();
    unsigned length;
    if (is_.Read( reinterpret_cast<char*>(&length), 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be32( length );
    ParseArray( length );
}

void MessagePackReader::ParseArray(size_t length)
{
    log_debug("MessagePackReader::ParseArray, length=" << length);
    ResetToken();
    handler_->StartArray(length);

    for (size_t i=0; i<length; i++)
    {
        ParseStream();
        if (i != (length-1))
            handler_->ArraySeparator();
    }
    handler_->EndArray(length);
}

void MessagePackReader::ParseMap16()
{
    log_trace();
    unsigned short length;
    if (is_.Read( reinterpret_cast<char*>(&length), 2 ) != 2)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be16( length );  // compiling for mingw had problem when this was combined with the next line
    ParseMap( length );
}

void MessagePackReader::ParseMap32()
{
    log_trace();
    unsigned length;
    if (is_.Read( reinterpret_cast<char*>(&length), 4 ) != 4)
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    length = _msgpack_be32( length );
    ParseMap( length );
}

void MessagePackReader::ParseMap(size_t length)
{
    log_debug("MessagePackReader::ParseMap, length=" << length);
    ResetToken();
    handler_->StartMap(length);

    for (size_t i=0; i<length; i++)
    {
        ParseKey();
        ParseStream();
        if (i != (length-1))
            handler_->MapSeparator();
    }
    handler_->EndMap(length);
}

}
