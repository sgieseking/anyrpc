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

#ifndef ANYRPC_JSONREADER_H_
#define ANYRPC_JSONREADER_H_

namespace anyrpc
{

//! Interpret a stream in Json format to generate construction calls to the handler
class ANYRPC_API JsonReader : public Reader
{
public:
    JsonReader(Stream& is) : Reader(is) {}
    virtual ~JsonReader() {}

    //! Parse the input stream to the handler
    virtual void ParseStream(Handler& handler);

private:
    //! Internal function that may be called recursively to parse the stream.
    void ParseStream();
    void ParseValue();
    void ParseNull();
    void ParseTrue();
    void ParseFalse();
    void ParseString();
    void ParseKey();
    //! Parse the input stream to an output stream as a string that needs decoded.
    void ParseStringToStream(Stream& os);
    unsigned ParseHex4();
    void EncodeUtf8(Stream& os, unsigned codepoint);
    void ParseMap();
    void ParseArray();
    void ParseNumber();

    log_define("AnyRPC.JsonReader");
};

} // namespace anyrpc

#endif // ANYRPC_JSONREADER_H_
