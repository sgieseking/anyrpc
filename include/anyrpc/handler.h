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

#ifndef ANYRPC_HANDLER_H_
#define ANYRPC_HANDLER_H_

namespace anyrpc
{

//! Receive events during parsing to create a new representation.
/*!
 *  The Handler class is used to process the events that are created
 *  during the parsing process.  This is used to either create an internal
 *  document or to write a document in a specific format.
 */
class ANYRPC_API Handler
{
public:
    Handler() {}
    virtual ~Handler() {}

    //!@name Document Member Functions
    //@{
    virtual void StartDocument() {}
    virtual void EndDocument() {}
    //@}

    //!@name Miscellaneous Member Functions
    //@{
    virtual void Null() = 0;

    virtual void BoolTrue() = 0;
    virtual void BoolFalse() = 0;

    virtual void DateTime(time_t dt) = 0;

    virtual void String(const char* str, std::size_t length, bool copy = true) = 0;
    virtual void Binary(const unsigned char* str, std::size_t length, bool copy = true) = 0;
    //@}

    //!@name Number Member Functions
    //@{
    virtual void Int(int i) = 0;
    virtual void Uint(unsigned u) = 0;
    virtual void Int64(int64_t i64) = 0;
    virtual void Uint64(uint64_t u64) = 0;
    virtual void Float(float f) { Double( (double)f ); }
    virtual void Double(double d) = 0;
    //@}

    //!@name Map Processing Member Functions
    //@{
    virtual void StartMap() { anyrpc_throw(AnyRpcErrorIllegalCall,"Illegal call"); }
    virtual void StartMap(std::size_t /* memberCount */) { return StartMap(); }
    virtual void Key(const char* str, std::size_t length, bool copy = true) = 0;
    virtual void MapSeparator() {}
    virtual void EndMap(std::size_t memberCount = 0) = 0;
    //@}

    //!@name Array Processing Member Functions
    //@{
    virtual void StartArray() { anyrpc_throw(AnyRpcErrorIllegalCall,"Illegal call"); };
    virtual void StartArray(std::size_t /* elementCount */) { return StartArray(); }
    virtual void ArraySeparator() {}
    virtual void EndArray(std::size_t elementCount = 0) = 0;
    //@}

protected:
    log_define("AnyRPC.Handler");

private:
    // Prohibit copy constructor & assignment operator.
    Handler(const Handler&);
    Handler& operator=(const Handler&);
};

} // namespace anyrpc

////////////////////////////////////////////////////////////////////////////////

ANYRPC_API anyrpc::Handler& operator<<(anyrpc::Handler& handler, const anyrpc::Value& value);

#endif // ANYRPC_HANDLER_H_
