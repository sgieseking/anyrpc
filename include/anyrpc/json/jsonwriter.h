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

#ifndef ANYRPC_JSONWRITER_H_
#define ANYRPC_JSONWRITER_H_

namespace anyrpc
{

//! Convert the data in the output stream using Json format
/*!
 *  The JsonWriter uses the calls to the handler and writes the equivalent
 *  text stream in Json format.  Typically, this is performed when
 *  traversing a Document/Value.
 *
 *  Call types that are not directly supported by Json
 *  (DateTime, Binary) are converted into two element arrays with the
 *  first element indicating the type and the second element containing
 *  the data.  This can be automatically converted back when read
 *  and parsed into a document.
 *
 *  The only encoding types currently support are UTF and ASCII.
 *  With ASCII encoding all of the extended characters are output using
 *  escaped format, \uXXXX.
 */
class ANYRPC_API JsonWriter : public Handler
{
public:
    JsonWriter(Stream& os, EncodingEnum encoding=UTF8) : os_(os), encoding_(encoding) {}
    virtual ~JsonWriter() {}

    //!@name Miscellaneous Member Functions
    //@{
    virtual void Null();

    virtual void BoolTrue();
    virtual void BoolFalse();

    virtual void DateTime(time_t dt);

    virtual void String(const char* str, std::size_t length, bool copy = true);
    virtual void Binary(const unsigned char* str, std::size_t length, bool copy = true);
    //@}

    //!@name Number Member Functions
    //@{
    virtual void Int(int i);
    virtual void Uint(unsigned u);
    virtual void Int64(int64_t i64);
    virtual void Uint64(uint64_t u64);
    virtual void Double(double d);
    //@}

    //!@name Map Processing Member Functions
    //@{
    virtual void StartMap();
    virtual void Key(const char* str, std::size_t length, bool copy = true);
    virtual void MapSeparator();
    virtual void EndMap(std::size_t memberCount = 0);
    //@}

    //!@name Array Processing Member Functions
    //@{
    virtual void StartArray();
    virtual void ArraySeparator();
    virtual void EndArray(std::size_t elementCount = 0);
    //@}

private:
    //! convert from UTF8 to Unicode character
    void DecodeUtf8(const char* str, std::size_t length, std::size_t &pos, unsigned &codepoint);

    Stream& os_;                    //!< Stream to write the json representation to
    EncodingEnum encoding_;         //!< Encoding format for stream

    log_define("AnyRPC.JsonWriter");
};

} // namespace anyrpc

#endif // ANYRPC_JSONWRITER_H_
