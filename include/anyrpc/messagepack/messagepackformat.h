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

#ifndef ANYRPC_MESSAGEPACKFORMAT_H_
#define ANYRPC_MESSAGEPACKFORMAT_H_

namespace anyrpc
{

const unsigned char MessagePackPosFixInt     = 0x00;
const unsigned char MessagePackFixMap        = 0x80;
const unsigned char MessagePackFixArray      = 0x90;
const unsigned char MessagePackFixStr        = 0xa0;
const unsigned char MessagePackNil           = 0xc0;
const unsigned char MessagePackFalse         = 0xc2;
const unsigned char MessagePackTrue          = 0xc3;
const unsigned char MessagePackBin8          = 0xc4;
const unsigned char MessagePackBin16         = 0xc5;
const unsigned char MessagePackBin32         = 0xc6;
const unsigned char MessagePackExt8          = 0xc7;
const unsigned char MessagePackExt16         = 0xc8;
const unsigned char MessagePackExt32         = 0xc9;
const unsigned char MessagePackFloat32       = 0xca;
const unsigned char MessagePackFloat64       = 0xcb;
const unsigned char MessagePackUint8         = 0xcc;
const unsigned char MessagePackUint16        = 0xcd;
const unsigned char MessagePackUint32        = 0xce;
const unsigned char MessagePackUint64        = 0xcf;
const unsigned char MessagePackInt8          = 0xd0;
const unsigned char MessagePackInt16         = 0xd1;
const unsigned char MessagePackInt32         = 0xd2;
const unsigned char MessagePackInt64         = 0xd3;
const unsigned char MessagePackFixExt1       = 0xd4;
const unsigned char MessagePackFixExt2       = 0xd5;
const unsigned char MessagePackFixExt4       = 0xd6;
const unsigned char MessagePackFixExt8       = 0xd7;
const unsigned char MessagePackFixExt16      = 0xd8;
const unsigned char MessagePackStr8          = 0xd9;
const unsigned char MessagePackStr16         = 0xda;
const unsigned char MessagePackStr32         = 0xdb;
const unsigned char MessagePackArray16       = 0xdc;
const unsigned char MessagePackArray32       = 0xdd;
const unsigned char MessagePackMap16         = 0xde;
const unsigned char MessagePackMap32         = 0xdf;
const unsigned char MessagePackNegFixInt     = 0xe0;

} // namespace anyrpc

#endif // ANYRPC_MESSAGEPACKFORMAT_H_
