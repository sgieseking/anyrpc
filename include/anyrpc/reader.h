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

#ifndef ANYRPC_READER_H_
#define ANYRPC_READER_H_

namespace anyrpc
{

//! The Reader class is the base class for reading streams into a handler.

class ANYRPC_API Reader
{
public:
    Reader(Stream& is) : is_(is), handler_(0), inSitu_(is.InSitu()), copy_(is.UseStringCopy()) {}
    virtual ~Reader() {}

    //! Parse the stream through to the handler until the document is complete or an error occurs
    virtual void ParseStream(Handler& handler) = 0;

    //! Indicate whether an error has occurred during the processing
    bool HasParseError() const { return parseError_.IsErrorSet(); }
    int GetParseErrorCode() const { return parseError_.GetCode(); }
    const std::string& GetParseErrorStr() const { return parseError_.GetMessage(); }
    std::size_t GetErrorOffset() const { return parseError_.GetOffset(); }

protected:
    //! Set a parse error with the given code and at the given offset in the stream
    void SetParseError(const AnyRpcException& fault) { parseError_ = fault; }

    //! Skip standard whitespace characters
    void SkipWhiteSpace();

    virtual bool IsWhiteSpace(char c) { return (std::isspace(c) != 0); }

    static const int DefaultParseReserve = 50;      //!< Default space when expecting a short string

    Stream& is_;
    Handler* handler_;
    AnyRpcException parseError_;
    bool inSitu_;
    bool copy_;

    log_define("AnyRPC.Reader");
};
} // namespace anyrpc

////////////////////////////////////////////////////////////////////////////////

ANYRPC_API anyrpc::Reader& operator>>(anyrpc::Reader& reader, anyrpc::Handler& handler);

#endif // ANYRPC_READER_H_
