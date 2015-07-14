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

#ifndef ANYRPC_BASE64_H_
#define ANYRPC_BASE64_H_

namespace anyrpc
{
namespace internal
{

//! Perform Base64 encoding on the binary data.
/*!
 *  Put the result in the output Stream, os.
 */
void Base64Encode(Stream& os, const unsigned char* encode, std::size_t length);

//! Perform Base64 decoding on the input Stream, is, until the terminating character, termChar, is reached.
/*!
 *  Put the result in the output Stream, os.
 *  Return whether the decoding was successful.
 */
bool Base64Decode(Stream& os, Stream &is, char termChar);

//! Perform Base64 decoding on the source data, src, to the destination, dest.
/*!
 *  It is assumed that the destination has sufficient space allocate.
 *  Return the number of byte in the destination or zero if there is some type of error.
 */
std::size_t Base64Decode(unsigned char* dest, unsigned char* src, std::size_t srcLength);

} // namespace internal
} // namespace anyrpc

#endif // ANYRPC_BASE64_H_
