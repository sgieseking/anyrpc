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
#include "anyrpc/document.h"

namespace anyrpc
{

void Document::StartDocument()
{
    log_debug("StartDocument");
    value_.SetInvalid();
    stack_.clear();
    stack_.push_back( &value_ );
}

void Document::Null()
{
    log_debug("Null");
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetNull();
}

void Document::BoolTrue()
{
    log_debug("BoolTrue");
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetBool(true);
}

void Document::BoolFalse()
{
    log_debug("BoolFalse");
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetBool(false);
}

void Document::Int(int i)
{
    log_debug("Int: " << i);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetInt(i);
}

void Document::Uint(unsigned u)
{
    log_debug("Uint: " << u);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetUint(u);
}

void Document::Int64(int64_t i64)
{
    log_debug("Int64: " << i64);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetInt64(i64);
}

void Document::Uint64(uint64_t u64)
{
    log_debug("Uint64: " << u64);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetUint64(u64);
}

void Document::Double(double d)
{
    log_debug("Double: " << d);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetDouble(d);
}

void Document::DateTime(time_t dt)
{
    log_debug("DateTime: " << dt);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetDateTime(dt);
}

void Document::String(const char* str, size_t length, bool copy)
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
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetString(str, length, copy);
}

void Document::Binary(const unsigned char* str, size_t length, bool copy)
{
    log_debug("Binary: length=" << length);
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetBinary(str, length, copy);
}

void Document::StartMap()
{
    log_debug("StartMap");
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
    stack_.back()->SetMap();
}

void Document::Key(const char* str, size_t length, bool copy)
{
    log_debug("Key: length=" << length << ", str=" << str);
    anyrpc_assert( stack_.back()->IsMap(), AnyRpcErrorValueAccess, "Not Map, type=" << stack_.back()->GetType() );
    Value& value = stack_.back()->AddMember(str, length, copy);
    stack_.push_back(&value);
}

void Document::MapSeparator()
{
    log_debug("MapSeparator");
    stack_.pop_back();
    anyrpc_assert( stack_.back()->IsMap(), AnyRpcErrorValueAccess, "Not Map, type=" << stack_.back()->GetType() );
}

void Document::EndMap(size_t memberCount)
{
    log_debug("EndMap: count=" << memberCount);
    //if (!stack_.back()->IsMap())
    if (memberCount != 0)
        stack_.pop_back();
    anyrpc_assert( stack_.back()->MemberCount() == memberCount, AnyRpcErrorMapCountWrong,
            "Member counts different, call=" << memberCount << ", value=" << stack_.back()->MemberCount());
}

void Document::StartArray()
{
    log_debug("StartArray");
    anyrpc_assert( stack_.back()->IsInvalid(), AnyRpcErrorAccessNotInvalidValue, "Not invalid, type=" << stack_.back()->GetType() );
     // make it an array
    stack_.back()->SetArray();
    // add a Invalid member
    Value value;
    stack_.back()->PushBack(value);
    // get a reference to this value
    stack_.push_back(&stack_.back()->at(0));
}

void Document::ArraySeparator()
{
    log_debug("ArraySeparator");
    anyrpc_assert( stack_.back()->IsValid(), AnyRpcErrorAccessInvalidValue, "Not valid, type=" << stack_.back()->GetType() );
    stack_.pop_back();
    int size = stack_.back()->Size();
    Value value;
    stack_.back()->PushBack(value);
    stack_.push_back(&stack_.back()->at(size));
}

void Document::EndArray(size_t elementCount)
{
    log_debug("EndArray: count=" << elementCount);
    if (stack_.back()->IsInvalid())
    {
        // no elements in the array but there is the invalid value to handle
        stack_.pop_back();
        anyrpc_assert( stack_.back()->Size() == 1, AnyRpcErrorArrayCountWrong, "Expected size of 1, size=" << stack_.back()->Size() );
        stack_.back()->SetSize(0);
    }
    else
    {
        // just pop the last value from the stack
        stack_.pop_back();

        // Check if this is an anyrpc extension to automatically convert
        if (convertExtensions_ &&
            (stack_.back()->Size() >= 2) &&
            (stack_.back()->at(0).IsString()) )
        {
            const char* str = stack_.back()->at(0).GetString();
            size_t length = stack_.back()->at(0).GetStringLength();

            if ((strncmp(str,ANYRPC_DATETIME_STRING,length) == 0) && (stack_.back()->at(1).IsString()))
                ConvertDateTime(stack_.back());

            else if ((strncmp(str,ANYRPC_BASE64_STRING,length) == 0) && (stack_.back()->at(1).IsString()))
                ConvertBase64(stack_.back());
        }
        else
            anyrpc_assert( stack_.back()->Size() == elementCount, AnyRpcErrorArrayCountWrong,
                    "Expected size of " << elementCount << ", size=" << stack_.back()->Size());
    }
}

void Document::ConvertDateTime(Value *value)
{
    // convert string to time
    const char* str = value->at(1).GetString();
    size_t length = value->at(1).GetStringLength();
    struct tm t;
    if ((length != 17) ||
        (sscanf(str, "%4d%2d%2dT%2d:%2d:%2d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6))
        anyrpc_throw(AnyRpcErrorTermination, "Parsing was terminated");
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    t.tm_isdst = -1;
    //setenv("TZ", "UTC", 1); // set timezone to UTC
    time_t dt = mktime(&t);
    log_debug("ConvertDateTime: str=" << str << ", DateTime=" << dt);
    value->SetDateTime(dt);
}

void Document::ConvertBase64(Value *value)
{
    log_time(INFO);

    // transfer to a new Value so it doesn't get clobbered when changing the assignment
    Value newValue;
    newValue.Assign(value->at(1));
    value->Assign(newValue);

    if (!value->ConvertBase64())
        anyrpc_throw(AnyRpcErrorBase64Invalid, "Error during base64 decode");
}

void Document::EndDocument()
{
    log_debug("EndDocument");
    //log_assert( stack_.size() == 1, "Expected stack size of 1, but is " << stack_.size() );
    stack_.clear();
}

}
