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

#ifndef ANYRPC_MESSAGEPACKREADER_H_
#define ANYRPC_MESSAGEPACKREADER_H_

namespace anyrpc
{

//! Interpret a stream in MessagePack format to generate construction calls to the handler
/*!
 *  To support Insitu parsing of string which need to be null terminated, the next token
 *  is read early so that it can be replaced with the null terminator.  Since the string
 *  length is known due to the MessagePack format specification, the string is really just
 *  skipped.  The same skipping is performed for binary data but the binary data does not
 *  need to be null terminated.
 */
class ANYRPC_API MessagePackReader : public Reader
{
public:
    MessagePackReader(Stream& is) : Reader(is), handler_(0), token_(0) {}
    virtual ~MessagePackReader() {}

    virtual void ParseStream(Handler& handler);

private:
    //! Get the next token value which may have already been read
    void GetToken() { if (token_ == 0) token_ = is_.Get(); }
    //! Get the next token and clear the value in the insitu stream (i.e. null terminate string)
    void GetClearToken() { token_ = is_.GetClear(); }
    //! Clear the token to force a new read
    void ResetToken() { token_ = 0; }

    void ParseStream();
    void ParseKey();

    void ParseNull();
    void ParseFalse();
    void ParseTrue();

    void ParsePositiveFixInt();
    void ParseNegativeFixInt();
    void ParseUint8();
    void ParseUint16();
    void ParseUint32();
    void ParseUint64();
    void ParseInt8();
    void ParseInt16();
    void ParseInt32();
    void ParseInt64();

    void ParseFloat32();
    void ParseFloat64();

    void ParseFixStr();
    void ParseStr8();
    void ParseStr16();
    void ParseStr32();
    void ParseStr(std::size_t length);

    void ParseBin8();
    void ParseBin16();
    void ParseBin32();
    void ParseBin(std::size_t length);

    void ParseFixArray() { ParseArray(token_&0xf); }
    void ParseArray16();
    void ParseArray32();
    void ParseArray(std::size_t length);

    void ParseFixMap() { ParseMap(token_&0xf); }
    void ParseMap16();
    void ParseMap32();
    void ParseMap(std::size_t length);

    // Extension formats are not currently implemented
    void ParseFixExt1()  { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseFixExt2()  { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseFixExt4()  { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseFixExt8()  { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseFixExt16() { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseExt8()     { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseExt16()    { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }
    void ParseExt32()    { anyrpc_throw(AnyRpcErrorNotImplemented, "Feature not implemented"); }

    void ParseIllegal()  { anyrpc_throw(AnyRpcErrorValueInvalid, "Invalid value"); }

    typedef void (MessagePackReader::*Function)();
    static Function ParseLookup[256];

    Handler* handler_;
    unsigned char token_;

    log_define("AnyRPC.MpacReader");
};

} // namespace anyrpc

#endif // ANYRPC_MESSAGEPACKREADER_H_
