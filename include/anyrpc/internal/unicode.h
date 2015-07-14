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

#ifndef ANYRPC_UNICODE_H_
#define ANYRPC_UNICODE_H_

namespace anyrpc
{
namespace internal
{
std::string ConvertToUtf8(const wchar_t* ws);
std::string ConvertToUtf8(const wchar_t* ws, std::size_t length);

void ConvertToUtf8(const wchar_t* ws, std::string& s);
void ConvertToUtf8(const wchar_t* ws, std::size_t length, std::string& s);

std::wstring ConvertFromUtf8(const char* s);
std::wstring ConvertFromUtf8(const char* s, std::size_t length);

void ConvertFromUtf8(const char* s, std::wstring& ws);
void ConvertFromUtf8(const char* s, std::size_t length, std::wstring& ws);

} // namespace internal
} // namespace anyrpc

#endif // ANYRPC_UNICODE_H_
