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
#include "anyrpc/internal/unicode.h"

using namespace anyrpc::internal;

namespace anyrpc
{

//! Member structure for maps
struct Member
{
    Value key;              //!< key of member (must be a string)
    Value value;            //!< value of member.
};

////////////////////////////////////////////////////////////////////////////////

Value::Value() : data_(), flags_(0)
{
}

Value::Value(const Value& rhs) : data_(), flags_(0)
{
    log_debug("Copy Constructor");
    CopyInternal(rhs);
}

Value::Value(ValueType type)
{
    log_debug("Construct Type: " << type);
    static const unsigned defaultFlags[ValueTypeSize] =
            { InvalidFlag, NullFlag, FalseFlag, TrueFlag, MapFlag, ArrayFlag,
              ShortStringFlag, NumberAnyFlag, DateTimeType, ShortBinaryFlag };

    anyrpc_assert(type < ValueTypeSize, AnyRpcErrorValueAccess, "Illegal type=" << type);
    if (type >= ValueTypeSize)
        type = InvalidType;

    // Set default flags based on the type
    flags_ = defaultFlags[type];

    // zero all the data
    memset((void*)&data_, 0, sizeof(data_));

    // Use ShortString to store empty string or binary.
    if ((type == StringType) || (type == BinaryType))
        data_.ss.SetLength(0);
}

Value::Value(bool b) : data_(), flags_(b ? TrueFlag : FalseFlag)
{
    log_debug("Construct Bool: " << b);
}

Value::Value(int i) : flags_(NumberIntFlag)
{
    log_debug("Construct Int: " << i);
    data_.n.i64 = i;

    // Add additional flags as appropriate for the number that is given
    if (i >= 0)
        flags_ |= UintFlag | Uint64Flag;
}

Value::Value(unsigned u) : flags_(NumberUintFlag)
{
    log_debug("Construct Uint: " << u);
    data_.n.u64 = u;

    // Add additional flags as appropriate for the number that is given
    if (!(u & 0x80000000))
        flags_ |= IntFlag | Int64Flag;
}

Value::Value(int64_t i64) : flags_(NumberInt64Flag)
{
    log_debug("Construct Int64: " << i64);
    data_.n.i64 = i64;

    // Add additional flags as appropriate for the number that is given
    if (i64 >= 0)
    {
        flags_ |= NumberUint64Flag;
        if (!(static_cast<uint64_t>(i64) & ANYRPC_UINT64_C2(0xFFFFFFFF, 0x00000000)))
            flags_ |= UintFlag;
        if (!(static_cast<uint64_t>(i64) & ANYRPC_UINT64_C2(0xFFFFFFFF, 0x80000000)))
            flags_ |= IntFlag;
    }
    else if (i64 >= static_cast<int64_t>(ANYRPC_UINT64_C2(0xFFFFFFFF, 0x80000000)))
        flags_ |= IntFlag;
}

Value::Value(uint64_t u64) : flags_(NumberUint64Flag)
{
    log_debug("Construct Uint64: " << u64);
    data_.n.u64 = u64;

    // Add additional flags as appropriate for the number that is given
    if (!(u64 & ANYRPC_UINT64_C2(0x80000000, 0x00000000)))
        flags_ |= Int64Flag;
    if (!(u64 & ANYRPC_UINT64_C2(0xFFFFFFFF, 0x00000000)))
        flags_ |= UintFlag;
    if (!(u64 & ANYRPC_UINT64_C2(0xFFFFFFFF, 0x80000000)))
        flags_ |= IntFlag;
}

Value::Value(float f) : flags_(NumberFloatFlag)
{
    log_debug("Construct Float: " << f);
    data_.n.f = f;
}

Value::Value(double d) : flags_(NumberDoubleFlag)
{
    log_debug("Construct Double: " << d);
    data_.n.d = d;
}

Value::Value(const char* s, bool copy) : flags_(ConstStringFlag)
{
    size_t length = strlen(s);

#if BUILD_WITH_LOG4CPLUS
    if (length < 10)
        log_debug("Construct String: copy=" << copy << ", length=" << length << ", str=" << s);
    else
    {
        char logStr[11];
        memcpy(logStr,s,10);
        logStr[10] = 0;
        log_debug("Construct String: copy=" << copy << ", length=" << length << ", str=" << logStr << "...");
    }
#endif

    if (copy)
        CopyString(s,length);
    else
    {
        // Just point to the given string. It should be a constant at least over the lifetime of the Value.
        data_.s.str = s;
        data_.s.length = length;
    }
}

Value::Value(const char* s, std::size_t length, bool copy) : flags_(ConstStringFlag)
{
#if BUILD_WITH_LOG4CPLUS
    if (length < 10)
        log_debug("Construct String: copy=" << copy << ", length=" << length << ", str=" << s);
    else
    {
        char logStr[11];
        memcpy(logStr,s,10);
        logStr[10] = 0;
        log_debug("Construct String: copy=" << copy << ", length=" << length << ", str=" << logStr << "...");
    }
#endif

    if (copy)
        CopyString(s,length);
    else
    {
        anyrpc_assert(s[length] == 0, AnyRpcErrorStringNotTerminated, "String not null terminated");  // must be null terminated
        // Just point to the given string. It should be a constant at least over the lifetime of the Value.
        data_.s.str = s;
        data_.s.length = length;
    }
}

Value::Value(const std::string& s) : flags_(ConstStringFlag)
{
    size_t length = s.length();
#if BUILD_WITH_LOG4CPLUS
    if (length < 10)
        log_debug("Construct String: length=" << length << ", str=" << s.c_str());
    else
    {
        char logStr[11];
        memcpy(logStr,s.c_str(),10);
        logStr[10] = 0;
        log_debug("Construct String: length=" << length << ", str=" << logStr << "...");
    }
#endif

    CopyString(s.c_str(),length);
}

Value::Value(const unsigned char* s, std::size_t length, bool copy) : flags_(ConstBinaryFlag)
{
    log_debug("Construct Binary: copy=" << copy << ", length=" << length);

    if (copy)
        CopyBinary(s,length);
    else
    {
        // Just point to the given data. It should be a constant at least over the lifetime of the Value.
        data_.s.str = (const char*)(s);
        data_.s.length = length;
    }
}
#if defined(ANYRPC_WCHAR)
Value::Value(const wchar_t* ws)
{
    std::string s;
    ConvertToUtf8(ws, wcslen(ws), s);
    CopyString(s.c_str(), s.length());
}

Value::Value(const wchar_t* ws, size_t length)
{
    std::string s;
    ConvertToUtf8(ws, length, s);
    CopyString(s.c_str(), s.length());
}

Value::Value(const std::wstring& ws)
{
    std::string s;
    ConvertToUtf8(ws.c_str(), ws.length(), s);
    CopyString(s.c_str(), s.length());
}
#endif
Value::~Value()
{
    Destroy();
}

void Value::Destroy()
{
    if (IsValid())
    {
        if ((flags_ == CopyStringFlag) || (flags_ == CopyBinaryFlag))
        {
            // the data was allocated so it must be freed
            free((char*) data_.s.str);
        }
        else if (IsArray())
        {
            // destroy the elements and then free the allocated space
            for (Value* v = data_.a.elements; v != data_.a.elements + data_.a.size; ++v)
                v->~Value();
            free(data_.a.elements);
        }
        else if (IsMap())
        {
            // destroy the keys and values and then free the allocated space
            for (Member* m = data_.m.members; m != data_.m.members + data_.m.size; ++m)
            {
                m->key.~Value();
                m->value.~Value();
            }
            free(data_.m.members);
        }
    }

    flags_ = InvalidFlag;
}

Value& Value::operator=(const Value& rhs)
{
    log_debug("Copy Value");
    anyrpc_assert(this != &rhs, AnyRpcErrorIllegalAssignment, "Can't set equal to itself");
    Destroy();
    CopyInternal(rhs);
    return *this;
}

Value& Value::operator=(bool b)
{
    Destroy();
    new (this) Value(b);
    return *this;
}

Value& Value::operator=(int i)
{
    Destroy();
    new (this) Value(i);
    return *this;
}

Value& Value::operator=(unsigned u)
{
    Destroy();
    new (this) Value(u);
    return *this;
}

Value& Value::operator=(int64_t i64)
{
    Destroy();
    new (this) Value(i64);
    return *this;
}

Value& Value::operator=(uint64_t u64)
{
    Destroy();
    new (this) Value(u64);
    return *this;
}

Value& Value::operator=(float f)
{
    Destroy();
    new (this) Value(f);
    return *this;
}

Value& Value::operator=(double d)
{
    Destroy();
    new (this) Value(d);
    return *this;
}

Value& Value::operator=(const char* s)
{
    Destroy();
    new (this) Value(s,true);
    return *this;
}

Value& Value::operator=(const std::string& s)
{
    Destroy();
    new (this) Value(s);
    return *this;
}

#if defined(ANYRPC_WCHAR)
Value& Value::operator=(const wchar_t* ws)
{
    Destroy();
    new (this) Value(ws);
    return *this;
}

Value& Value::operator=(const std::wstring& ws)
{
    Destroy();
    new (this) Value(ws.c_str(),ws.length());
    return *this;
}
#endif

Value& Value::SetInvalid()
{
    log_debug("SetInvalid");
    Destroy();
    flags_ = InvalidFlag;
    return *this;
}

Value& Value::SetNull()
{
    log_debug("SetNull");
    Destroy();
    flags_ = NullFlag;
    return *this;
}

Value& Value::SetBool(bool b)
{
    log_debug("SetBool: " << b);
    Destroy();
    flags_ = (b ? TrueFlag : FalseFlag);
    return *this;
}

Value& Value::SetMap()
{
    log_debug("SetMap");
    Destroy();
    new (this) Value(MapType);
    return *this;
}

void Value::AddMemberCheckCapacity()
{
    if (IsInvalid())
        SetMap();
    else
        anyrpc_assert(IsMap(), AnyRpcErrorValueAccess, "Not map, type=" << GetType());

    Map& m = data_.m;
    if (m.size >= m.capacity)
    {
        if (m.capacity == 0)
        {
            m.capacity = DefaultMapCapacity;
            m.members = reinterpret_cast<Member*>(malloc(m.capacity*sizeof(Member)));
        }
        else
        {
            uint32_t newCapacity = m.capacity + (m.capacity + 3) / 4;  // grow by 25%, assumes this will still be 32-bits
            if (newCapacity > MaxMapCapacity)   // gcc didn't like using std::max() with MaxMapCapacity
                newCapacity = MaxMapCapacity;
            m.capacity = newCapacity;
            m.members = reinterpret_cast<Member*>(realloc(m.members, m.capacity * sizeof(Member)));
        }
    }
    anyrpc_assert(m.size < m.capacity, AnyRpcErrorMemoryAllocation, "Too many members, size=" << m.size << ", capacity=" << m.capacity);

    // initialize the next member as invalid
    data_.m.members[m.size].key.flags_ = InvalidFlag;
    data_.m.members[m.size].value.flags_ = InvalidFlag;
}

Value& Value::AddMember(Value& key, Value& value, bool copy)
{
    log_debug("AddMember");
    anyrpc_assert(key.IsString(), AnyRpcErrorValueAccess, "Key is not string, type=" << key.GetType());
    AddMemberCheckCapacity();
    data_.m.members[data_.m.size].key.Set(key,copy);
    data_.m.members[data_.m.size].value.Set(value,copy);
    data_.m.size++;
    return data_.m.members[data_.m.size-1].value;
}

Value& Value::AddMember(const char* str, std::size_t length, Value& value, bool copy)
{
    log_debug("AddMember");
    AddMemberCheckCapacity();
    data_.m.members[data_.m.size].key.SetString(str, length, copy);
    data_.m.members[data_.m.size].value.Set(value,copy);
    data_.m.size++;
    return data_.m.members[data_.m.size-1].value;
}

Value& Value::AddMember(Value& key, bool copy)
{
    log_debug("AddMember, key only");
    anyrpc_assert(key.IsString(), AnyRpcErrorValueAccess, "Key is not string, type=" << key.GetType());
    AddMemberCheckCapacity();
    data_.m.members[data_.m.size].key.Set(key,copy);
    data_.m.members[data_.m.size].value.SetInvalid();
    data_.m.size++;
    return data_.m.members[data_.m.size-1].value;
}

Value& Value::AddMember(const char* str, std::size_t length, bool copy)
{
    log_debug("AddMember, key only");
    AddMemberCheckCapacity();
    data_.m.members[data_.m.size].key.SetString(str, length, copy);
    data_.m.members[data_.m.size].value.SetInvalid();
    data_.m.size++;
    return data_.m.members[data_.m.size-1].value;
}

bool Value::HasMember(const char* str)
{
    return (FindMember(str) != MemberEnd());
}

bool Value::HasMember(const std::string& str)
{
    return (FindMember(str) != MemberEnd());
}

MemberIterator Value::FindMember(const Value& key)
{
    log_debug("FindMember");
    anyrpc_assert(key.IsString(), AnyRpcErrorValueAccess, "Key is not string, type=" << key.GetType());
    anyrpc_assert(IsMap() || IsInvalid(), AnyRpcErrorValueAccess, "Not map, type=" << GetType() << ", find=" << key.GetString());

    if (!IsMap())
        return MemberIterator(0);

    MemberIterator it;
    for (it=MemberBegin(); it!=MemberEnd(); it++)
        if (key.StringEqual(it->key))
            break;
    return it;
}

MemberIterator Value::FindMember(const char* str)
{
    Value key(str,false);
    return FindMember(key);
}

MemberIterator Value::FindMember(const std::string& str)
{
    Value key(str.c_str(),false);
    return FindMember(key);
}

Value& Value::operator[](const char* str)
{
    if (IsInvalid())
        return AddMember(str);

    MemberIterator it = FindMember(str);
    if (it != MemberEnd())
        return it->value;

    return AddMember(str);
}

MemberIterator Value::MemberBegin()
{
    anyrpc_assert(IsMap(), AnyRpcErrorValueAccess, "Not map, type=" << GetType());
    if (!IsMap()) return MemberIterator(0);
    return MemberIterator(data_.m.members);
}

MemberIterator Value::MemberEnd()
{
    anyrpc_assert(IsMap(), AnyRpcErrorValueAccess, "Not map, type=" << GetType());
    if (!IsMap()) return MemberIterator(0);
    return MemberIterator(data_.m.members + data_.m.size);
}

#if defined(ANYRPC_WCHAR)
Value& Value::AddMember(const wchar_t* ws, std::size_t length, Value& value, bool copy)
{
    log_debug("AddMember");
    AddMemberCheckCapacity();
    data_.m.members[data_.m.size].key.SetString(ws, length);
    data_.m.members[data_.m.size].value.Set(value,copy);
    data_.m.size++;
    return data_.m.members[data_.m.size-1].value;
}

Value& Value::AddMember(const wchar_t* ws, std::size_t length, bool copy)
{
    log_debug("AddMember, key only");
    AddMemberCheckCapacity();
    data_.m.members[data_.m.size].key.SetString(ws, length);
    data_.m.members[data_.m.size].value.SetInvalid();
    data_.m.size++;
    return data_.m.members[data_.m.size-1].value;
}

bool Value::HasMember(const wchar_t* ws)
{
    return (FindMember(ws) != MemberEnd());
}

bool Value::HasMember(const std::wstring& ws)
{
    return (FindMember(ws) != MemberEnd());
}

MemberIterator Value::FindMember(const wchar_t* ws)
{
    Value key(ws);
    return FindMember(key);
}

MemberIterator Value::FindMember(const std::wstring& ws)
{
    Value key(ws.c_str());
    return FindMember(key);
}

Value& Value::operator[](const wchar_t* ws)
{
    std::string str = ConvertToUtf8(ws, wcslen(ws));

    if (IsInvalid())
        return AddMember(str);

    MemberIterator it = FindMember(str);
    if (it != MemberEnd())
        return it->value;

    return AddMember(str);
}
#endif

Value& Value::SetArray()
{
    log_debug("SetArray");
    Destroy();
    new (this) Value(ArrayType);
    return *this;
}

Value& Value::SetArray(std::size_t capacity)
{
    log_debug("SetArray: capacity=" << capacity);
    Destroy();
    new (this) Value(ArrayType);

    anyrpc_assert(capacity < MaxArrayCapacity, AnyRpcErrorMemoryAllocation, "Too many elements, size=" << capacity << ", capacity=" << MaxArrayCapacity);

    // allocate the data and set to invalid
    data_.a.elements = (Value*)calloc(capacity, sizeof(Value));
    data_.a.capacity = static_cast<uint32_t>(capacity);
    data_.a.size = static_cast<uint32_t>(capacity);
    return *this;
}

void Value::Clear()
{
    log_debug("Clear");
    anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType());
    for (size_t i = 0; i < data_.a.size; ++i)
        data_.a.elements[i].~Value();
    data_.a.size = 0;
}

Value& Value::operator[](std::size_t index)
{
    if (IsInvalid())
        SetArray();
    anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType());
    if (index >= data_.a.size)
        SetSize(index+1);
    anyrpc_assert(index < data_.a.size, AnyRpcErrorIllegalArrayAccess, "Illegal index, size=" << data_.a.size << ", index=" << index);
    return data_.a.elements[index];
}

Value& Value::Reserve(size_t newCapacity)
{
    log_debug("Reserve: newCapacity=" << newCapacity);
    anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType());
    anyrpc_assert(newCapacity < MaxArrayCapacity, AnyRpcErrorMemoryAllocation, "Too many elements, size=" << newCapacity << ", capacity=" << MaxArrayCapacity);

    if (newCapacity > data_.a.capacity)
    {
        data_.a.elements = (Value*)realloc(data_.a.elements, newCapacity * sizeof(Value));
        data_.a.capacity = static_cast<uint32_t>(newCapacity);
    }
    return *this;
}

Value& Value::SetSize(size_t newSize)
{
    log_debug("SetSize: newSize=" << newSize);
    if (IsInvalid())
        SetArray();
    anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType());
    anyrpc_assert(newSize < MaxArrayCapacity, AnyRpcErrorMemoryAllocation, "Too many elements, size=" << newSize << ", capacity=" << MaxArrayCapacity);

    if (newSize > data_.a.capacity)
    {
        data_.a.elements = (Value*)realloc(data_.a.elements, newSize * sizeof(Value));
        data_.a.capacity = static_cast<uint32_t>(newSize);
    }
    if (newSize > data_.a.size)
    {
        for (size_t i=data_.a.size; i<newSize; i++)
            data_.a.elements[i].flags_ = InvalidFlag;
    }
    else
    {
        for (size_t i=newSize; i<data_.a.size; i++)
            data_.a.elements[i].~Value();
    }
    data_.a.size = static_cast<uint32_t>(newSize);
    return *this;
}

Value& Value::PushBack(Value& value)
{
    log_debug("PushBack");
    anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType());
    if (data_.a.size >= data_.a.capacity)
        Reserve( (data_.a.capacity == 0) ? DefaultArrayCapacity : (data_.a.capacity + (data_.a.capacity + 1) / 2));
    data_.a.elements[data_.a.size++].RawAssign(value);
    return *this;
}


float Value::GetFloat() const
{
    anyrpc_assert(IsNumber(), AnyRpcErrorValueAccess, "Not number, type=" << GetType());
    if ((flags_ & FloatFlag) != 0)
        return data_.n.f;   // exact type, no conversion.
    if ((flags_ & DoubleFlag) != 0)
        return (float) data_.n.d;   // double -> float (may lose precision)
    if ((flags_ & IntFlag) != 0)
        return (float) data_.n.i.i;   // int -> float (may lose precision)
    if ((flags_ & UintFlag) != 0)
        return (float) data_.n.u.u;   // unsigned -> float (may lose precision)
    if ((flags_ & Int64Flag) != 0)
        return (float) data_.n.i64;   // int64_t -> float (may lose precision)
    anyrpc_assert((flags_ & Uint64Flag) != 0, AnyRpcErrorValueAccess, "Unknown number type=" << flags_);
    return (float) data_.n.u64;   // uint64_t -> float (may lose precision)
}

double Value::GetDouble() const
{
    anyrpc_assert(IsNumber(), AnyRpcErrorValueAccess, "Not number, type=" << GetType());
    if ((flags_ & DoubleFlag) != 0)
        return data_.n.d;   // exact type, no conversion.
    if ((flags_ & FloatFlag) != 0)
        return data_.n.f;   // float -> double
    if ((flags_ & IntFlag) != 0)
        return data_.n.i.i;   // int -> double
    if ((flags_ & UintFlag) != 0)
        return data_.n.u.u;   // unsigned -> double
    if ((flags_ & Int64Flag) != 0)
        return (double) data_.n.i64;   // int64_t -> double (may lose precision)
    anyrpc_assert((flags_ & Uint64Flag) != 0, AnyRpcErrorValueAccess, "Unknown number type=" << flags_);
    return (double) data_.n.u64;   // uint64_t -> double (may lose precision)
}

Value& Value::SetInt(int i)
{
    Destroy();
    new (this) Value(i);
    return *this;
}

Value& Value::SetUint(unsigned u)
{
    Destroy();
    new (this) Value(u);
    return *this;
}

Value& Value::SetInt64(int64_t i64)
{
    Destroy();
    new (this) Value(i64);
    return *this;
}

Value& Value::SetUint64(uint64_t u64)
{
    Destroy();
    new (this) Value(u64);
    return *this;
}

Value& Value::SetFloat(float f)
{
    Destroy();
    new (this) Value(f);
    return *this;
}

Value& Value::SetDouble(double d)
{
    Destroy();
    new (this) Value(d);
    return *this;
}

Value& Value::SetDateTime(time_t dt)
{
    log_debug("SetDateTime: " << dt);
    Destroy();
    flags_ = DateTimeType;
    data_.dt = dt;
    return *this;
}

const char* Value::GetString() const
{
    anyrpc_assert(IsString(), AnyRpcErrorValueAccess, "Not String, type=" << GetType());
    return ((flags_ & InlineStrFlag) ? data_.ss.str : data_.s.str);
}

std::size_t Value::GetStringLength() const
{
    anyrpc_assert(IsString(), AnyRpcErrorValueAccess, "Not String, type=" << GetType());
    return ((flags_ & InlineStrFlag) ? (data_.ss.GetLength()) : data_.s.length);
}

Value& Value::SetString(const char* s, bool copy)
{
    Destroy();
    new (this) Value(s, copy);
    return *this;
}

Value& Value::SetString(const char* s, std::size_t length, bool copy)
{
    Destroy();
    new (this) Value(s, length, copy);
    return *this;
}
#if defined(ANYRPC_WCHAR)
std::wstring Value::GetWString() const
{
    anyrpc_assert(IsString(), AnyRpcErrorValueAccess, "Not String, type=" << GetType());
    const char* s = (flags_ & InlineStrFlag) ? data_.ss.str : data_.s.str;
    size_t length = (flags_ & InlineStrFlag) ? (data_.ss.GetLength()) : data_.s.length;
    return ConvertFromUtf8(s, length);
}

void Value::GetWString(std::wstring& ws) const
{
    anyrpc_assert(IsString(), AnyRpcErrorValueAccess, "Not String, type=" << GetType());
    const char* s = (flags_ & InlineStrFlag) ? data_.ss.str : data_.s.str;
    size_t length = (flags_ & InlineStrFlag) ? (data_.ss.GetLength()) : data_.s.length;
    ConvertFromUtf8(s, length, ws);
}

Value& Value::SetString(const wchar_t* ws)
{
    Destroy();
    new (this) Value(ws);
    return *this;
}

Value& Value::SetString(const wchar_t* ws, std::size_t length)
{
    Destroy();
    new (this) Value(ws, length);
    return *this;
}
#endif

const unsigned char* Value::GetBinary() const
{
    anyrpc_assert(IsBinary(), AnyRpcErrorValueAccess, "Not Binary, type=" << GetType());
    return (unsigned char*)((flags_ & InlineStrFlag) ? data_.ss.str : data_.s.str);
}

std::size_t Value::GetBinaryLength() const
{
    anyrpc_assert(IsBinary(), AnyRpcErrorValueAccess, "Not Binary, type=" << GetType());
    return ((flags_ & InlineStrFlag) ? (data_.ss.GetLength()) : data_.s.length);
}

Value& Value::SetBinary(const unsigned char* s, size_t length, bool copy)
{
    Destroy();
    new (this) Value(s, length, copy);
    return *this;
}

// Initialize this value as copy string with initial data, without calling destructor.
void Value::CopyString(const char* s, size_t length)
{
    char *str = 0;
    if (ShortString::Usable(length))
    {
        flags_ = ShortStringFlag;
        data_.ss.SetLength(length);
        str = data_.ss.str;
    }
    else
    {
        str = (char *) malloc((length + 1) * sizeof(char));
        anyrpc_assert(str != 0, AnyRpcErrorMemoryAllocation, "Data allocation failed");
        if (str == 0)
            return;
        flags_ = CopyStringFlag;
        data_.s.length = length;
        data_.s.str = str;
    }
    std::memcpy(str, s, length * sizeof(char));
    str[length] = '\0';
}

// Initialize this value as copy binary with initial data, without calling destructor.
void Value::CopyBinary(const unsigned char* s, size_t length)
{
    char *str = 0;
    if (ShortString::Usable(length))
    {
        flags_ = ShortBinaryFlag;
        data_.ss.SetLength(length);
        str = data_.ss.str;
    }
    else
    {
        str = (char *) malloc(length);
        anyrpc_assert(str != 0, AnyRpcErrorMemoryAllocation, "Data allocation failed");
        if (str == 0)
            return;
        flags_ = CopyBinaryFlag;
        data_.s.length = length;
        data_.s.str = str;
    }
    std::memcpy(str, s, length);
}

// Assignment without calling destructor
void Value::RawAssign(Value& rhs)
{
    data_ = rhs.data_;
    flags_ = rhs.flags_;
    rhs.flags_ = NullFlag;
}

bool Value::StringEqual(const Value& rhs) const
{
    anyrpc_assert(IsString(), AnyRpcErrorValueAccess, "Not String, type=" << GetType());
    anyrpc_assert(rhs.IsString(), AnyRpcErrorValueAccess, "RHS not String, type=" << GetType());

    const size_t len1 = GetStringLength();
    const size_t len2 = rhs.GetStringLength();
    if (len1 != len2)
        return false;

    const char* const str1 = GetString();
    const char* const str2 = rhs.GetString();
    if (str1 == str2)  // fast path for constant string
        return true;

    return (std::memcmp(str1, str2, sizeof(char) * len1) == 0);
}

std::ostream& Value::WriteStream(std::ostream& os) const
{
    log_time(INFO);
    return WriteStreamInternal(os);
}

std::ostream& Value::WriteStreamInternal(std::ostream& os) const
{
    switch (GetType())
    {
        case InvalidType:
            os << "invalid";
            break;
        case NullType:
            os << "null";
            break;
        case FalseType:
            os << "false";
            break;
        case TrueType:
            os << "true";
            break;
        case NumberType:
            if (IsInt())            // must be checked before IsInt64()
                os << data_.n.i.i;
            else if (IsUint())      // must be checked before IsUint64()
                os << data_.n.u.u;
            else if (IsInt64())
                os << data_.n.i64;
            else if (IsUint64())
                os << data_.n.u64;
            else if (IsDouble())    // must be checked before float
                os << data_.n.d;
            else
                os << data_.n.f;
            break;
        case DateTimeType:
        {
            char buffer[100];
            struct tm t;
            localtime_r(&data_.dt, &t);
            //gmtime_r(&data_.dt, &t);
            snprintf(buffer, sizeof(buffer), "%4d%02d%02dT%02d:%02d:%02dZ",
                    1900+t.tm_year, 1+t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
            os << buffer;
            break;
        }
        case StringType:
            os << '\"' << GetString() << '\"';
            break;
        case BinaryType:
        {
            char buffer[5];
            os << '(';
            const unsigned char* str = GetBinary();
            size_t len = GetBinaryLength();
            for (size_t i=0; i<len; i++)
            {
                snprintf(buffer, sizeof(buffer), "%02x", *(str+i));
                os << buffer;
            }
            os << ')';
            break;
        }
        case ArrayType:
            os << "[";
            for (size_t i=0; i<data_.a.size; i++)
            {
                data_.a.elements[i].WriteStreamInternal(os);
                if (i != (data_.a.size-1))
                    os << ",";
            }
            os << "]";
            break;
        case MapType:
            os << "{";
            for (size_t i=0; i<data_.m.size; i++)
            {
                data_.m.members[i].key.WriteStreamInternal(os);
                os << ":";
                data_.m.members[i].value.WriteStreamInternal(os);
                if (i != (data_.m.size-1))
                    os << ",";
            }
            os << "}";
            break;
        default:
            break;
    }

    return os;
}

void Value::Traverse(Handler& handler) const
{
    log_time(INFO);
    TraverseInternal(handler);
}

void Value::TraverseInternal(Handler& handler) const
{
    switch (GetType())
    {
        case InvalidType:
            anyrpc_assert(false, AnyRpcErrorAccessInvalidValue, "Invalid type during traverse");
            break;
        case NullType:
            handler.Null();
            break;
        case FalseType:
            handler.BoolFalse();
            break;
        case TrueType:
            handler.BoolTrue();
            break;
        case NumberType:
            if (IsInt())            // must be checked before IsInt64()
                handler.Int( data_.n.i.i );
            else if (IsUint())      // must be checked before IsUint64()
                handler.Uint( data_.n.u.u );
            else if (IsInt64())
                handler.Int64( data_.n.i64 );
            else if (IsUint64())
                handler.Uint64( data_.n.u64 );
            else if (IsDouble())    // must be checked before float
                handler.Double( data_.n.d );
            else
                handler.Float( data_.n.f );
            break;
        case DateTimeType:
            handler.DateTime( data_.dt );
            break;
        case StringType:
            handler.String( GetString(), GetStringLength() );
            break;
        case BinaryType:
            handler.Binary( GetBinary(), GetBinaryLength() );
            break;
        case ArrayType:
            handler.StartArray(data_.a.size);
            for (size_t i=0; i<data_.a.size; i++)
            {
                data_.a.elements[i].TraverseInternal(handler);
                if (i != (data_.a.size-1))
                    handler.ArraySeparator();
            }
            handler.EndArray(data_.a.size);
            break;
        case MapType:
            handler.StartMap(data_.m.size);
            for (size_t i=0; i<data_.m.size; i++)
            {
                Member *member = &data_.m.members[i];
                anyrpc_assert(member->key.IsString(), AnyRpcErrorValueAccess, "Key is not string, type=" << member->key.GetType());
                handler.Key( member->key.GetString(), member->key.GetStringLength() );
                member->value.TraverseInternal(handler);
                if (i != (data_.m.size-1))
                    handler.MapSeparator();
            }
            handler.EndMap(data_.m.size);
            break;
        default:
            break;
    }
}

void Value::Set(Value& value, bool copy)
{
    if (copy)
        CopyInternal(value);
    else
        RawAssign(value);
}

void Value::Copy(const Value& value)
{
    log_time(INFO);
    CopyInternal(value);
}

void Value::CopyInternal(const Value& value)
{
    switch (value.GetType())
    {
        case InvalidType:
        case NullType:
        case FalseType:
        case TrueType:
        case NumberType:
        case DateTimeType:
            data_ = value.data_;
            flags_ = value.flags_;
            break;
        case StringType:
            SetString( (value.flags_ & InlineStrFlag) ? value.data_.ss.str : value.data_.s.str,
                       (value.flags_ & InlineStrFlag) ? value.data_.ss.GetLength() : value.data_.s.length );
            break;
        case BinaryType:
            SetBinary( (const unsigned char*)((value.flags_ & InlineStrFlag) ? value.data_.ss.str : value.data_.s.str),
                       (value.flags_ & InlineStrFlag) ? value.data_.ss.GetLength() : value.data_.s.length );
            break;
        case ArrayType:
            SetArray(value.data_.a.size);
            for (size_t i=0; i<value.data_.a.size; i++)
                data_.a.elements[i].CopyInternal(value.data_.a.elements[i]);
            break;
        case MapType:
            SetMap();
            for (size_t i=0; i<value.data_.m.size; i++)
                AddMember(value.data_.m.members[i].key.GetString(), value.data_.m.members[i].value);
            break;
        default:
            break;
    }
}

void Value::Assign(Value& value)
{
    if (this != &value)
    {
        Destroy();
        RawAssign(value);
    }
}

bool Value::ConvertBase64()
{
    log_time(INFO);
    anyrpc_assert(IsString(), AnyRpcErrorValueAccess, "Not string, type=" << GetType());
    if (!IsString())
    {
        log_fatal("Not a string");
        return false;
    }

    unsigned char* str = (unsigned char*)GetString();
    size_t length = GetStringLength();

    size_t convertedLength = Base64Decode(str, str, length);
    if (convertedLength == 0)
    {
        log_fatal("Failed to convert from base64");
        return false;
    }

    // convert to binary type
    flags_ = (flags_ & (CopyFlag | InlineStrFlag)) | BinaryType | BinaryFlag;

    if (flags_ == ShortBinaryFlag)
        data_.ss.SetLength(convertedLength);
    else
    {
        data_.s.length = convertedLength;
        if (flags_ == CopyBinaryFlag)
        {
            char* newStr = static_cast<char*>(realloc(str, convertedLength));
            if (newStr != 0)
                data_.s.str = newStr;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

MemberIterator MemberIterator::operator++()
{
    ptr_++;
    return *this;
}

MemberIterator MemberIterator::operator++(int junk)
{
    MemberIterator i=*this;
    ptr_++;
    return i;
}

MemberIterator MemberIterator::operator--()
{
    ptr_--;
    return *this;
}

MemberIterator MemberIterator::operator--(int junk)
{
    MemberIterator i=*this;
    ptr_--;
    return i;
}

Value& MemberIterator::GetKey()
{
    return ptr_->key;
}

Value& MemberIterator::GetValue()
{
    return ptr_->value;
}

} // namespace anyrpc

// ostream
std::ostream& operator<<(std::ostream& os, anyrpc::Value& v)
{
    return v.WriteStream(os);
}

