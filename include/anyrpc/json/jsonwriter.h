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
    JsonWriter(Stream& os, EncodingEnum encoding=UTF8, unsigned precision=0, bool pretty=false) :
        os_(os), encoding_(encoding), pretty_(pretty), level_(0), precision_(precision) {}
    virtual ~JsonWriter() {}

    //! Make the text more readable with indentation and linefeeds
    void SetPretty(bool pretty=true) { pretty_ = pretty; }
    //! Set the double format method
    void SetScientificPrecision(unsigned precision)
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
    virtual void StartArray(size_t elementCount);
    virtual void ArraySeparator();
    virtual void EndArray(std::size_t elementCount = 0);
    //@}

private:
    //!@name Pretty Output Member Functions
    //@{
    inline void NewLine();
    inline void IncLevel();
    inline void DecLevel();
    //@}

    //! convert from UTF8 to Unicode character
    void DecodeUtf8(const char* str, std::size_t length, std::size_t &pos, unsigned &codepoint);

    Stream& os_;                    //!< Stream to write the json representation to
    EncodingEnum encoding_;         //!< Encoding format for stream
    bool pretty_;                   //!< Write using tabs and newlines to make it easier for a person to read
    int level_;                     //!< Current indent level
    unsigned precision_;            //!< Number of digits of precision when using scientific notation for doubles

    log_define("AnyRPC.JsonWriter");
};

//! Convert value to Json string
ANYRPC_API std::string ToJsonString(Value& value, EncodingEnum encoding=UTF8, unsigned precision=12, bool pretty=false);

} // namespace anyrpc

#endif // ANYRPC_JSONWRITER_H_
