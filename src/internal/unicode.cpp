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

#include "anyrpc/api.h"
#include "anyrpc/logger.h"
#include "anyrpc/error.h"
#include "anyrpc/internal/unicode.h"

namespace anyrpc
{
namespace internal
{

log_define("AnyRPC.Unicode");

std::string ConvertToUtf8(const wchar_t* ws)
{
    std::string s;
    ConvertToUtf8(ws, wcslen(ws), s);
    return s;
}

std::string ConvertToUtf8(const wchar_t* ws, size_t length)
{
    std::string s;
    ConvertToUtf8(ws, length, s);
    return s;
}

void ConvertToUtf8(const wchar_t* ws, std::string& s)
{
    ConvertToUtf8(ws, wcslen(ws), s);
}

void ConvertToUtf8(const wchar_t* ws, size_t length, std::string& s)
{
    for(size_t i=0; i<length; i++)
    {
        wchar_t wc = *ws++;

        if (wc <= 0x7F)
            s.push_back(static_cast<char>(wc));
        else if (wc <= 0x7FF)
        {
            s.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0xFF)));
            s.push_back(static_cast<char>(0x80 | ((wc     ) & 0x3F)));
        }
#if defined(WIN32)
    	// check for surrogate pair
        else if ((wc >= 0xD800) && (wc <= 0xDBFF))
		{
        	i++;
        	if (i >= length)
        		anyrpc_throw(AnyRpcErrorSurrogatePair, "Invalid surrogate pair");
        	unsigned cp1 = wc;
        	unsigned cp2 = *ws++;
        	if ((cp2 < 0xDC00) || (cp2 > 0xDFFF))
        		anyrpc_throw(AnyRpcErrorSurrogatePair, "Invalid surrogate pair");
        	unsigned cp = ((cp1 - 0xD800) << 10) + (cp2 - 0xDC00) + 0x10000;
            s.push_back(static_cast<char>(0xF0 | ((cp >>18) & 0xFF)));
            s.push_back(static_cast<char>(0x80 | ((cp >>12) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((cp     ) & 0x3F)));
		}
        else
        {
            s.push_back(static_cast<char>(0xE0 | ((wc >>12) & 0xFF)));
            s.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((wc     ) & 0x3F)));
        }
#else
        else if (wc <= 0xFFFF)
        {
            s.push_back(static_cast<char>(0xE0 | ((wc >>12) & 0xFF)));
            s.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((wc     ) & 0x3F)));
        }
        else if (wc <= 0x1FFFF)
        {
            s.push_back(static_cast<char>(0xF0 | ((wc >>18) & 0xFF)));
            s.push_back(static_cast<char>(0x80 | ((wc >>12) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((wc     ) & 0x3F)));
        }
        else
            anyrpc_throw(AnyRpcErrorUnicodeValue, "Invalid codepoint");
#endif
    }
}

std::wstring ConvertFromUtf8(const char* s)
{
    std::wstring ws;
    ConvertFromUtf8(s, strlen(s), ws);
    return ws;
}

std::wstring ConvertFromUtf8(const char* s, size_t length)
{
    std::wstring ws;
    ConvertFromUtf8(s, length, ws);
    return ws;
}

void ConvertFromUtf8(const char* s, std::wstring& ws)
{
    ConvertFromUtf8(s, strlen(s), ws);
}

void ConvertFromUtf8(const char* s, size_t length, std::wstring& ws)
{
    while (length > 0)
    {
        wchar_t wc;
        unsigned char c = *s++;

        if (c < 0x80)
        {
            // single byte sequence
            wc = c;
            length--;
        }
        else if ((c < 0xC0) || (c >= 0xF8))
            anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
        else if (c < 0xE0)
        {
            // two byte sequence
            if (length < 2)
                anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
            wchar_t c1 = c;
            wchar_t c2 = *s++;
            length -= 2;
            if ((c2 & 0xC0) != 0x80)
                anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
            wc = ((c1 & 0x1F) << 6) + (c2 & 0x3F);
        }
        else if (c < 0xF0)
        {
            // three byte sequence
            if (length < 3)
                anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
            wchar_t c1 = c;
            wchar_t c2 = *s++;
            wchar_t c3 = *s++;
            length -= 3;
            if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80))
                anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
            wc = ((c1 & 0x0F) << 12) + ((c2 & 0x3F) << 6) + (c3 & 0x3F);
        }
        else
        {
            // four byte sequence
            if (length < 4)
                anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
            wchar_t c1 = c;
            wchar_t c2 = *s++;
            wchar_t c3 = *s++;
            wchar_t c4 = *s++;
            length -= 4;
            if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80))
                anyrpc_throw(AnyRpcErrorUtf8Sequence, "Invalid utf8 sequence");
            wc = ((c1 & 0x07) << 18) + ((c2 & 0x3F) << 12) + ((c3 & 0x3F) << 6) + (c4 & 0x3F);
#if defined(WIN32)
            // add as surrogate pair
            ws.push_back( (wc >> 10) + 0xD800 );
            wc = (wc & 0x03FF) + 0xDC00;
#endif
        }
        ws.push_back(wc);
    }
}

} // namespace internal
} // namespace anyrpc
