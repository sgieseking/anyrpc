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

#ifndef ANYRPC_MESSAGEPACKWRITER_H_
#define ANYRPC_MESSAGEPACKWRITER_H_

namespace anyrpc
{

//! Convert the data in the output stream using MessagePack format
/*!
 *  The MessagePackWriter uses the calls to the handler and writes the equivalent
 *  text stream in MessagePack format.  Typically, this is performed when
 *  traversing a Document/Value.
 *
 *  Call types that are not directly supported by MessagePack
 *  (DateTime) are converted into two element arrays with the
 *  first element indicating the type and the second element containing
 *  the data.  This can be automatically converted back when read
 *  and parsed into a document.
 */
class ANYRPC_API MessagePackWriter : public Handler
{
public:
    MessagePackWriter(Stream& os) : os_(os) {}
    virtual ~MessagePackWriter() {}

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
    virtual void Float(float f);
    virtual void Double(double d);
    //@}

    //!@name Map Processing Member Functions
    //@{
    virtual void StartMap(std::size_t memberCount);
    virtual void Key(const char* str, std::size_t length, bool copy = true);
    virtual void EndMap(std::size_t memberCount = 0);
    //@}

    //!@name Array Processing Member Functions
    //@{
    virtual void StartArray(std::size_t elementCount);
    virtual void EndArray(std::size_t elementCount = 0);
    //@}

private:
    Stream& os_;                    //!< Stream to write the messagepack representation to

    //! Write an unsigned number to the output stream
    void WriteUint(unsigned u);
    //! Write an unsigned 64bit number to the output stream
    void WriteUint64(uint64_t u64);

    log_define("AnyRPC.MpacWriter");
};

} // namespace anyrpc

#endif // ANYRPC_MESSAGEPACKWRITER_H_
