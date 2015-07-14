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

#ifndef ANYRPC_XMLREADER_H_
#define ANYRPC_XMLREADER_H_

namespace anyrpc
{

//! Interpret a stream in Xmlrpc format to generate construction calls to the handler
class ANYRPC_API XmlReader : public Reader
{
public:
    XmlReader(Stream& is) : Reader(is), tagSkipFirstChar_(false) {}
    virtual ~XmlReader() {}

    //! Parse the input stream to the handler
    virtual void ParseStream(Handler& handler);
    //! Parse the input stream find the method name
    std::string ParseMethod();
    //! Parse the input stream for parameters
    void ParseParams(Handler& handler);
    //! Parse the input stream as a response
    void ParseResponse(Handler& handler);

private:
    void ParseParams(bool paramsParsed=false);
    void ParseStream();
    //! Get the next tag from the input stream and return as the XmlTagEnum value
    int GetNextTag();
    void ParseValue(bool skipValueTag=false);
    void ParseNil();
    void ParseBoolean();
    void ParseNumber(int tag);
    void ParseString(int tag);
    void ParseKey();
    //! Parse the input stream to an output stream as a string that needs decoded.
    void ParseStringToStream(Stream& os);
    void ParseHexEscapeCode(Stream& os, const char* str, int length);
    void ParseDecimalEscapeCode(Stream& os, const char* str, int length);
    void EncodeUtf8(Stream& os, unsigned codepoint);
    void ParseMap();
    void ParseArray();
    void ParseDateTime();
    void ParseBase64();

    bool tagSkipFirstChar_;

    log_define("AnyRPC.XmlReader");
};

} // namespace anyrpc

#endif // ANYRPC_XMLREADER_H_
