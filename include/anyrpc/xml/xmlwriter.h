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

#ifndef ANYRPC_XMLWRITER_H_
#define ANYRPC_XMLWRITER_H_

namespace anyrpc
{

//! Convert the data in the output stream using Xmlrpc format
/*!
 *  The XmlWriter uses the calls to the handler and writes the equivalent
 *  text stream in Xmlrpc format.  Typically, this is performed when
 *  traversing a Document/Value.
 *
 *  The XmlWriter uses the common extensions for the Null value and for
 *  Int64/Uint64 formats.
 */
class ANYRPC_API XmlWriter : public Handler
{
public:
    XmlWriter(Stream& os, bool pretty=false) : os_(os), pretty_(pretty), level_(0), precision_(0) {}
    virtual ~XmlWriter() {}

    //! Make the text more readable with indentation and linefeeds
    void SetPretty(bool pretty=true) { pretty_ = pretty; }
    //! Set the double format method. The default is no exponents for xmlrpc spec compatibility.
    void SetScientificPrecision(unsigned precision=32)
        { precision_ = std::min( 32u, precision); }

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
    virtual void StartArray(std::size_t elementCount);
    virtual void ArraySeparator();
    virtual void EndArray(std::size_t elementCount = 0);
    //@}

private:
    //!@name Pretty Output Member Functions
    //@{
    void StringData(const char* str, std::size_t length);
    void StartLine();
    void StartToken(const char* pToken);
    void EndToken(const char* pToken);
    //@}

    //! Convert a double precision value to a string without using exponents
    void DoubleToStream(Stream& os, double value);
    inline unsigned LeadDigit(double arg, double precision)
        { return (std::min)( (unsigned)9, (unsigned)(arg + precision)); }

    Stream& os_;                    //!< Stream to write the messagepack representation to
    bool pretty_;                   //!< Write using tabs and newlines to make it easier for a person to read
    int level_;                     //!< Current indent level
    unsigned precision_;            //!< Number of digits of precision when using scientific notation for doubles

    log_define("AnyRPC.XmlWriter");
};

} // namespace anyrpc

#endif // ANYRPC_XMLWRITER_H_
