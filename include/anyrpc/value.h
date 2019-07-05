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

#ifndef ANYRPC_VALUE_H_
#define ANYRPC_VALUE_H_

namespace anyrpc
{

//! Type of JSON value
enum ANYRPC_API ValueType
{
    InvalidType,
    NullType,
    FalseType,
    TrueType,
    MapType,
    ArrayType,
    StringType,
    NumberType,
    DateTimeType,
    BinaryType,
    ValueTypeSize
};

// forward declarations
struct Member;
class Handler;
class MemberIterator;

///////////////////////////////////////////////////////////////////////////////
// Value

//! Represents a value (number, string, map, array, etc.).
/*!
 *  A value can be one of 9 types.  This class is a variant type supporting
 *  each of these types.  The goal is to minimize the space allocated for each
 *  object.  The base value object will occupy 16 bytes on a 32-bit architecture
 *  and 24 bytes on a 64-bit architecture.
 *
 *  Map and Array types have a separate allocation to store the individual elements
 *  that are needed for the type.
 *
 *  String and Binary types can use the value object to store short strings, allocate
 *  storage for a separate copy of the data, or reference the data.  If the data is
 *  referenced, then it is not freed on exit and must be maintained external to the
 *  value object.
 */
class ANYRPC_API Value
{
public:
    //!@name Constructors/Destructor
    //@{
    //! Default constructor creates an invalid value.
    Value();
    //! Copy constructor will duplicate the value structure.
    Value(const Value& rhs);
    //! Constructor for a specific value type with default content.
    explicit Value(ValueType type);
    //! Constructor for a boolean value type.
    explicit Value(bool b);
    //! Constructor for an integer value type.
    explicit Value(int i);
    //! Constructor for an unsigned value type.
    explicit Value(unsigned u);
    //! Constructor for a 64-bit integer value type.
    explicit Value(int64_t i64);
    //! Constructor for a 64-bit unsigned value type.
    explicit Value(uint64_t u64);
    //! Constructor for a float value type.
    explicit Value(float f);
    //! Constructor for a double value type.
    explicit Value(double d);
    //! Constructor for a string value type.  The string must be null terminated.
    explicit Value(const char* s, bool copy=true);
    //! Constructor for a string value type with a given length.  Strings that are not copied must be null terminated.
    explicit Value(const char* s, std::size_t length, bool copy=true);
    //! Constructor for a string value type.
    explicit Value(const std::string& s);
    //! Constructor for a binary value type with a given length.
    explicit Value(const unsigned char* s, std::size_t length, bool copy=true);
#if defined(ANYRPC_WCHAR)
    //! Constructor for a string value type from wide string.  The string must be null terminated
    explicit Value(const wchar_t* ws);
    //! Constructor for a string value type from wide string with a given length.
    explicit Value(const wchar_t* ws, std::size_t length);
    //! Constructor for a string value type from wide string.
    explicit Value(const std::wstring& ws);
#endif

    //! Destructor
    /*! Any allocated memory for a map, array, or string/binary will be freed.  This may require recursive calls.
      */
    ~Value();
    //@}

    //!@name Assignment Operators
    //@{
    Value& operator=(const Value& rhs);
    Value& operator=(bool b);
    Value& operator=(int i);
    Value& operator=(unsigned u);
    Value& operator=(int64_t i64);
    Value& operator=(uint64_t u64);
    Value& operator=(float f);
    Value& operator=(double d);
    Value& operator=(const char* s);
    Value& operator=(const std::string& s);
#if defined(ANYRPC_WCHAR)
    Value& operator=(const wchar_t* ws);
    Value& operator=(const std::wstring& ws);
#endif
    //@}

    //!@name Type Member Functions
    //@{
    inline ValueType GetType()  const { return static_cast<ValueType>(flags_ & TypeMask); }
    inline bool IsValid()       const { return flags_ != InvalidFlag; }
    inline bool IsInvalid()     const { return flags_ == InvalidFlag; }
    inline bool IsNull()        const { return flags_ == NullFlag; }
    inline bool IsFalse()       const { return flags_ == FalseFlag; }
    inline bool IsTrue()        const { return flags_ == TrueFlag; }
    inline bool IsBool()        const { return (flags_ & BoolFlag) != 0; }
    inline bool IsMap()         const { return GetType() == MapType; }
    inline bool IsArray()       const { return GetType() == ArrayType; }
    inline bool IsNumber()      const { return (flags_ & NumberFlag) != 0; }
    inline bool IsInt()         const { return (flags_ & IntFlag) != 0; }
    inline bool IsUint()        const { return (flags_ & UintFlag) != 0; }
    inline bool IsInt64()       const { return (flags_ & Int64Flag) != 0; }
    inline bool IsUint64()      const { return (flags_ & Uint64Flag) != 0; }
    inline bool IsFloat()       const { return (flags_ & FloatFlag) != 0; }
    inline bool IsDouble()      const { return (flags_ & DoubleFlag) != 0; }
    inline bool IsString()      const { return (flags_ & StringFlag) != 0; }
    inline bool IsDateTime()    const { return flags_ == DateTimeType; }
    inline bool IsBinary()      const { return (flags_ & BinaryFlag) != 0; }
    //@}

    //!@name Invalid Member Functions
    //@{
    Value& SetInvalid();
    //@}

    //!@name Null Member Functions
    //@{
    Value& SetNull();
    //@}

    //!@name Bool Member Functions
    //@{
    bool GetBool() const { anyrpc_assert(IsBool(), AnyRpcErrorValueAccess, "Not bool, type=" << GetType());  return flags_ == TrueFlag; }
    Value& SetBool(bool b);
    //@}

    //!@name Map Member Functions
    //! A map associates a string (key) with a value.  The value can be any type.
    //! The keys should be unique although this is not verified.
    //! The current implementation searches the keys sequentially.
    //! The AddMember functions return a reference to the value of the newly added member.
    //@{
    //! Set the object to be a map type.
    Value& SetMap();
    //! Add member with key and value.  The key must be a String.
    Value& AddMember(Value& key, Value& value, bool copy=true);
    //! Add member with string key and value.
    Value& AddMember(const char* str, std::size_t length, Value& value, bool copy=true);
    //! Add member with string key and value.
    Value& AddMember(const char* str, Value& value, bool copy=true) { return AddMember(str, strlen(str), value, copy); }
    //! Add member with string key and value.
    Value& AddMember(const std::string& str, Value& value) { return AddMember(str.c_str(), str.length(), value); }
    //! Add member with key. The key must be a String. The value will be invalid until assigned.
    Value& AddMember(Value& key, bool copy=true);
    //! Add member with string key.  The value will be invalid until assigned.
    Value& AddMember(const char* str, std::size_t length, bool copy=true);
    //! Add member with string key.  The value will be invalid until assigned.
    Value& AddMember(const char* str, bool copy=true) { return AddMember(str, strlen(str), copy); }
    //! Add member with string key.  The value will be invalid until assigned.
    Value& AddMember(const std::string& str) { return AddMember(str.c_str(), str.length()); }
    //! Indicate whether a map has a member with a given string key
    bool HasMember(const char* str);
    //! Indicate whether a map has a member with a given string key
    bool HasMember(const std::string& str);
    //! Find the member with the given key.  The key must be a string.  NULL is returned if the key is not found.
    MemberIterator FindMember(const Value& key);
    //! Find the member with the given string key.  NULL is returned if the key is not found.
    MemberIterator FindMember(const char* str);
    //! Find the member with the given string key.  NULL is returned if the key is not found.
    MemberIterator FindMember(const std::string& str);
    //! Return the number of members in the map.
    std::size_t MemberCount() const { anyrpc_assert(IsMap(), AnyRpcErrorValueAccess, "Not Map, type=" << GetType()); return data_.m.size; }
    //! Return whether the map has zero members.
    bool IsMapEmpty() const { anyrpc_assert(IsMap(), AnyRpcErrorValueAccess, "NotMap, type=" << GetType()); return data_.m.size == 0; }
    //! Access member with the given string key.  If the key is not found, create a new entry with an invalid value.
    Value& operator[](const char* str);
    //! Access member with the given string key.  If the key is not found, create a new entry with an invalid value.
    Value& operator[](const std::string& str) { return (*this)[str.c_str()]; }
    //! Iterator for the start element of a Map
    MemberIterator MemberBegin();
    //! Iterator for past the end of the elements of a Map
    MemberIterator MemberEnd();
#if defined(ANYRPC_WCHAR)
    //! Add member with wide string key and value.
    Value& AddMember(const wchar_t* ws, std::size_t length, Value& value, bool copy=true);
    //! Add member with wide string key and value.
    Value& AddMember(const wchar_t* ws, Value& value, bool copy=true) { return AddMember(ws, wcslen(ws), value, copy); }
    //! Add member with wide string key and value.
    Value& AddMember(const std::wstring& ws, Value& value, bool copy=true) { return AddMember(ws.c_str(), ws.length(), value, copy); }
    //! Add member with wide string key.  The value will be invalid until assigned.
    Value& AddMember(const wchar_t* ws, std::size_t length, bool copy=true);
    //! Add member with wide string key.  The value will be invalid until assigned.
    Value& AddMember(const wchar_t* ws, bool copy=true) { return AddMember(ws, wcslen(ws), copy); }
    //! Add member with wide string key.  The value will be invalid until assigned.
    Value& AddMember(const std::wstring& ws, bool copy=true) { return AddMember(ws.c_str(), ws.length(), copy); }
    //! Indicate whether a map has a member with a given wide string key
    bool HasMember(const wchar_t* ws);
    //! Indicate whether a map has a member with a given wide string key
    bool HasMember(const std::wstring& ws);
    //! Access member with the given wide string key.  If the key is not found, create a new entry with an invalid value.
    //! Find the member with the given wide string key.  NULL is returned if the key is not found.
    MemberIterator FindMember(const wchar_t* ws);
    //! Find the member with the given wide string key.  NULL is returned if the key is not found.
    MemberIterator FindMember(const std::wstring& ws);
    Value& operator[](const wchar_t* ws);
    //! Access member with the given wide string key.  If the key is not found, create a new entry with an invalid value.
    Value& operator[](const std::wstring& ws) { return (*this)[ws.c_str()]; }
#endif
    //@}

    //!@name Array Member Functions
    //@{
    //! Set the object to be an array type.
    Value& SetArray();
    //! Set the size of the array to a given capacity and size.  All values will be of invalid type.
    Value& SetArray(std::size_t capacity);
    //! Return the current size of the array.  This is the number of active elements.
    std::size_t Size() const { anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType()); return data_.a.size; }
    //! Return the current capacity of the array.  This is the number of allocated elements.
    std::size_t Capacity() const { anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType()); return data_.a.capacity; }
    //! Return whether the array has a size of zero.
    bool IsArrayEmpty() const { anyrpc_assert(IsArray(), AnyRpcErrorValueAccess, "Not Array, type=" << GetType()); return data_.a.size == 0; }
    //! Set the array size to zero.
    void Clear();
    //! Access a given element of the array.  The array will be increased to this size if the number is larger than the current size.
    Value& operator[](std::size_t index);
    //! Access a given element of the array.  The array will be increased to this size if the number is larger than the current size.
    Value& operator[](int index) { return (*this)[static_cast<std::size_t>(index)]; }
    //! Access a given element of the array.  The array will be increased to this size if the number is larger than the current size.
    Value& at(std::size_t index) { return (*this)[index]; }
    //! Access a given element of the array.  The array will be increased to this size if the number is larger than the current size.
    Value& at(int index) { return (*this)[static_cast<std::size_t>(index)]; }
    //! Set the capacity of the array.  This may decrease the number of elements.
    Value& Reserve(std::size_t newCapacity);
    //! Set the size of the array.  This may increase or decrease the number of elements.
    Value& SetSize(std::size_t newSize);
    //! Add a new value to the end of the array increasing the size by one.
    Value& PushBack(Value& value);
    //@}

    //!@name Number Member Functions
    //@{
    int GetInt()            const { anyrpc_assert(flags_ & IntFlag,    AnyRpcErrorValueAccess, "Not Int, type=" << GetType());    return data_.n.i.i; }
    unsigned GetUint()      const { anyrpc_assert(flags_ & UintFlag,   AnyRpcErrorValueAccess, "Not Uint, type=" << GetType());   return data_.n.u.u; }
    int64_t GetInt64()      const { anyrpc_assert(flags_ & Int64Flag,  AnyRpcErrorValueAccess, "Not Int64, type=" << GetType());  return data_.n.i64; }
    uint64_t GetUint64()    const { anyrpc_assert(flags_ & Uint64Flag, AnyRpcErrorValueAccess, "Not Uint64, type=" << GetType()); return data_.n.u64; }
    float GetFloat()        const;
    double GetDouble()      const;

    Value& SetInt(int i);
    Value& SetUint(unsigned u);
    Value& SetInt64(int64_t i64);
    Value& SetUint64(uint64_t u64);
    Value& SetFloat(float f);
    Value& SetDouble(double d);
    //@}

    //!@name DateTime Member Functions
    //! The DataTime is considered to be a local time and not a UTC time.
    //@{
    time_t GetDateTime() const { anyrpc_assert(IsDateTime(), AnyRpcErrorValueAccess, "Not DateType, type=" << GetType());  return data_.dt; }
    Value& SetDateTime(time_t dt);
    Value& SetDateTime() { return SetDateTime(time(NULL)); }
    //@}

    //!@name String Member Functions
    //@{
    const char* GetString() const;
    std::size_t GetStringLength() const;
    Value& SetString(const char* s, bool copy=true);
    Value& SetString(const char* s, std::size_t length, bool copy=true);
#if defined(ANYRPC_WCHAR)
    std::wstring GetWString() const;
    void GetWString(std::wstring& ws) const;
    Value& SetString(const wchar_t* ws);
    Value& SetString(const wchar_t* ws, std::size_t length);
#endif
    //@}

    //!@name Binary Member Functions
    //@{
    const unsigned char* GetBinary() const;
    std::size_t GetBinaryLength() const;
    Value& SetBinary(const unsigned char* s, std::size_t length, bool copy=true);
    //@}

    //!@name Traversal Member Functions
    //@{
    //! Write the current object to the ostream using mostly Json style formatting.
    std::ostream& WriteStream(std::ostream& os) const;
    //! Traverse the current object and generate calls to the handler.
    void Traverse(Handler& handler) const;
    //@}

    //!@name Assignment Member Functions
    //@{
    //! Set the object to the value either by duplicating or changing the assignment.
    void Set(Value& value, bool copy=true);
    //! Set the object to the value by duplicating
    void Copy(const Value& value);
    //! Set the object to the value by assigning.  The value will be null after.
    void Assign(Value& value);
    //@}

    //!@name Conversion Member Functions
    //@{
    //! Convert the String from Base64 to Binary. This conversion is performed in place on the allocated memory.
    bool ConvertBase64();
    //@}

protected:
    log_define("AnyRPC.Value");

private:

    // Attribution:  The structure for the data storage is modeled on that used by rapidJson.
    enum ValueFlags
    {
        BoolFlag       = 0x00000100,   //!< Is TrueType or FalseType
        NumberFlag     = 0x00000200,   //!< Is a number type - specialized by the following flags
        IntFlag        = 0x00000400,   //!< Is an integer type - access with data_.n.i.i
        UintFlag       = 0x00000800,   //!< Is an unsigned type - access with data_.n.u.u
        Int64Flag      = 0x00001000,   //!< Is an int64 type - access with data_.n.i64
        Uint64Flag     = 0x00002000,   //!< Is an uint64 type - access with data_.n.u64
        FloatFlag      = 0x00004000,   //!< Is a float type - access with data_.n.f
        DoubleFlag     = 0x00008000,   //!< Is a double type - access with data_.n.d
        StringFlag     = 0x00100000,   //!< Is a string - either short or standard
        CopyFlag       = 0x00200000,   //!< String must be copied to clone - either short string or malloced space
        InlineStrFlag  = 0x00400000,   //!< Short string - access with data_.ss.str
        BinaryFlag     = 0x00800000,   //!< Is binary data - either short or standard
    };

    enum ValueCompositeFlags
    {
        InvalidFlag        = InvalidType,
        NullFlag           = NullType,
        TrueFlag           = TrueType | BoolFlag,
        FalseFlag          = FalseType | BoolFlag,
        NumberIntFlag      = NumberType | NumberFlag | IntFlag | Int64Flag,
        NumberUintFlag     = NumberType | NumberFlag | UintFlag | Uint64Flag | Int64Flag,
        NumberInt64Flag    = NumberType | NumberFlag | Int64Flag,
        NumberUint64Flag   = NumberType | NumberFlag | Uint64Flag,
        NumberFloatFlag    = NumberType | NumberFlag | FloatFlag,
        NumberDoubleFlag   = NumberType | NumberFlag | FloatFlag | DoubleFlag,
        NumberAnyFlag      = NumberType | NumberFlag | IntFlag | Int64Flag | UintFlag | Uint64Flag | FloatFlag | DoubleFlag,
        ConstStringFlag    = StringType | StringFlag,
        CopyStringFlag     = StringType | StringFlag | CopyFlag,
        ShortStringFlag    = StringType | StringFlag | CopyFlag | InlineStrFlag,
        ConstBinaryFlag    = BinaryType | BinaryFlag,
        CopyBinaryFlag     = BinaryType | BinaryFlag | CopyFlag,
        ShortBinaryFlag    = BinaryType | BinaryFlag | CopyFlag | InlineStrFlag,
        MapFlag            = MapType,
        ArrayFlag          = ArrayType,
    };

    static const int TypeMask = 0xFF;

    static const std::size_t    DefaultArrayCapacity    = 16;
    static const std::size_t    MaxArrayCapacity        = 16*1024*1024;     // must be less than 2^32 / sizeof(Value) on 32-bit compilation
    static const uint32_t       DefaultMapCapacity      = 16;
    static const uint32_t       MaxMapCapacity          = 16*1024*1024;     // must be less than 2^32 / sizeof(Member) on 32-bit compilation

    //! Allocated or referenced string, part of Data union: 8 bytes in 32-bit mode, 16 bytes in 64-bit mode
    struct String
    {
        const char* str;
        std::size_t length;
    };

    // implementation detail: ShortString can represent zero-terminated strings up to MaxSize chars
    // (excluding the terminating zero) and store a value to determine the length of the contained
    // string in the last character str[LenPos] by storing "MaxSize - length" there. If the string
    // to store has the maximal length of MaxSize then str[LenPos] will be 0 and therefore act as
    // the string terminator as well. For getting the string length back from that value just use
    // "MaxSize - str[LenPos]".
    // This allows to store 11-chars strings in 32-bit mode and 15-chars strings in 64-bit mode
    // inline (for `UTF8`-encoded strings).

    //! Short string, part of Data union: at most as many bytes as "String" above => 12 bytes in 32-bit mode, 16 bytes in 64-bit mode
    class ShortString
    {
    public:
        enum
        {
            MaxChars = sizeof(String) / sizeof(char),
            MaxSize = MaxChars - 1,
            LenPos = MaxSize
        };
        char str[MaxChars];

        inline static bool Usable(std::size_t len) { return (MaxSize >= len); }
        inline void SetLength(std::size_t len) { str[LenPos] = (char) (MaxSize - len); }
        inline std::size_t GetLength() const { return (std::size_t) (MaxSize - str[LenPos]); }
    };

    // By using proper binary layout, retrieval of different integer types do not need conversions.
    // Both doubles and floats are supported but most text RPC protocols only support doubles.
    // Floats are provided mainly for binary protocols that might want to conserve space.

    //! Any number type, part of Data union: 8 bytes
    union Number
    {
#if ANYRPC_ENDIAN == ANYRPC_LITTLEENDIAN
        struct I
        {
            int i;
            char padding[4];
        } i;
        struct U
        {
            unsigned u;
            char padding2[4];
        } u;
#else
        struct I
        {
            char padding[4];
            int i;
        }i;
        struct U
        {
            char padding2[4];
            unsigned u;
        }u;
#endif
        int64_t i64;
        uint64_t u64;
        float f;
        double d;
    };

    //! Map type, part of Data union: 12 bytes in 32-bit mode, 16 bytes in 64-bit mode
    struct Map
    {
        Member* members;
        uint32_t size;
        uint32_t capacity;
    };

    //! Map type, part of Data union: 12 bytes in 32-bit mode, 16 bytes in 64-bit mode
    struct Array
    {
        Value* elements;
        uint32_t size;
        uint32_t capacity;
    };

    //! Union of each of the basic data types: 12 bytes in 32-bit mode, 16 bytes in 64-bit mode
    union Data
    {
        String s;
        ShortString ss;
        Number n;
        Map m;
        Array a;
        time_t dt;
    };

    //! Check the capacity of a map to allow a member to be added.
    void AddMemberCheckCapacity();
    //! Copy the string as either a short string or with allocated memory.
    void CopyString(const char* s) { CopyString(s, strlen(s)); }
    //! Copy the string as either a short string or with allocated memory.
    void CopyString(const char* s, size_t length);
    //! Copy the binary data as either a short string or with allocated memory.
    void CopyBinary(const unsigned char* s, size_t length);
    //! Copy the value without destroying the original data.
    void RawAssign(Value& rhs);
    //! Compare the given String value to the current object.
    bool StringEqual(const Value& rhs) const;

    //! Internal member function to perform the recursive calls to write to a stream.
    std::ostream& WriteStreamInternal(std::ostream& os) const;
    //! Internal member function to perform the recursive calls to traverse a value to a handler.
    void TraverseInternal(Handler& handler) const;
    //! Internal member function to perform the recursive calls to perform a deep copy.
    void CopyInternal(const Value& rhs);

    //! Delete any data and set to invalid
    void Destroy();

    Data data_;             //!< Data storage which depends on the flags
    unsigned flags_;        //!< Flags that indicate the type information for accessing the data
};  // 16 bytes in 32-bit mode, 24 bytes in 64-bit mode

////////////////////////////////////////////////////////////////////////////////

//! Iterator class to step through the members of a Map
/*!
 *  Implement iterator to step through the members of a map similar to std::map.
 *  Access to the members are also provided through access functions.
 */
class ANYRPC_API MemberIterator
{
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = Member;
	using difference_type = ptrdiff_t;
	using pointer = Member*;
	using reference = Member&;

    MemberIterator() : ptr_(0) {}
    MemberIterator(pointer ptr) : ptr_(ptr) {}
    MemberIterator(const MemberIterator& mit) : ptr_(mit.ptr_) {}

    //! @name dereference
    //@{
    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }
    //@}

    //! @name stepping
    //@{
    MemberIterator operator++();
    MemberIterator operator++(int);
    MemberIterator operator--();
    MemberIterator operator--(int);
    //@}

    //! @name relations
    //@{
    bool operator==(const MemberIterator& rhs) const { return ptr_ == rhs.ptr_; }
    bool operator!=(const MemberIterator& rhs) const { return ptr_ != rhs.ptr_; }
    //@}

    //! @name member access
    //!{
    Value& GetKey();        //!< Access the key from the member
    Value& GetValue();      //!< Access the value from the member
    //@}

private:
    pointer ptr_;
};

} // namespace anyrpc

////////////////////////////////////////////////////////////////////////////////

//! Write value to ostream using mostly Json style formatting.
ANYRPC_API std::ostream& operator<<(std::ostream& os, anyrpc::Value& v);

#endif // ANYRPC_VALUE_H_
