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
#include "anyrpc/value.h"
#include "anyrpc/stream.h"
#include "anyrpc/internal/base64.h"

namespace anyrpc
{
namespace internal
{

static const char base64Chars[] =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static const unsigned char base64Pad = 64;
static const unsigned char base64Ignore = 65;

static const unsigned char base64Decode[] = {
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // 00-0f
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // 10-1f
    65,65,65,65, 65,65,65,65, 65,65,65,62, 65,65,65,63,  // 20-2f
    52,53,54,55, 56,57,58,59, 60,61,65,65, 65,64,65,65,  // 30-3f
    65, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,  // 40-4f
    15,16,17,18, 19,20,21,22, 23,24,25,65, 65,65,65,65,  // 50-5f
    65,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,  // 60-6f
    41,42,43,44, 45,46,47,48, 49,50,51,65, 65,65,65,65,  // 70-7f
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // 80-8f
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // 90-9f
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // a0-af
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // b0-bf
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // c0-cf
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // d0-df
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // e0-ef
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,  // f0-ff
};

static const int maxLineLength = 64;
static const int maxQuadLength = maxLineLength / 4;

void Base64Encode(Stream& os, const unsigned char* encode, size_t length)
{
    unsigned char data0, data1, data2;
    char out[4];

    while (length > 2)
    {
        int quad;
        for (quad=0; (quad < maxQuadLength) && (length > 2); quad++, length-= 3)
        {
            data0 = *encode++;
            data1 = *encode++;
            data2 = *encode++;
            out[0] = base64Chars[   data0 >> 2 ];
            out[1] = base64Chars[ ((data0 & 0x03) << 4) + (data1 >> 4) ];
            out[2] = base64Chars[ ((data1 & 0x0f) << 2) + (data2 >> 6) ];
            out[3] = base64Chars[   data2 & 0x3f ];
            os.Put( out, 4 );
        }
        if ((quad == maxQuadLength) && (length > 0))
            os.Put('\n');
    }
    if (length == 2)
    {
        data0 = *encode++;
        data1 = *encode++;
        data2 = 9;
        out[0] = base64Chars[   data0 >> 2 ];
        out[1] = base64Chars[ ((data0 & 0x03) << 4) + (data1 >> 4) ];
        out[2] = base64Chars[ ((data1 & 0x0f) << 2) + (data2 >> 6) ];
        out[3] = '=';
        os.Put( out, 4 );
    }
    else if (length == 1)
    {
        data0 = *encode++;
        data1 = 0;
        data2 = 0;
        out[0] = base64Chars[   data0 >> 2 ];
        out[1] = base64Chars[ ((data0 & 0x03) << 4) + (data1 >> 4) ];
        out[2] = '=';
        out[3] = '=';
        os.Put( out, 4 );
    }
}

bool Base64Decode(Stream& os, Stream &is, char termChar)
{
    unsigned char data[4];
    char out[3];
    int avail = 0;
    while (!is.Eof())
    {
        if (is.Peek() == termChar)
            return (avail == 0);
        data[avail] = base64Decode[static_cast<unsigned char>(is.Get())];
        if (data[avail] == base64Ignore)
            continue;
        if (data[avail] == base64Pad)
        {
            if (avail < 2)
                return false;
            if (avail == 2)
            {
                // verify that next character is a pad as well
                if (base64Decode[static_cast<unsigned char>(is.Get())] != base64Pad)
                    return false;
                // decode one bytes
                os.Put( (data[0] << 2) + (data[1] >> 4));
            }
            else
            {
                // decode two bytes
                out[0] = ( data[0]         << 2) + (data[1] >> 4);
                out[1] = ((data[1] & 0x0f) << 4) + (data[2] >> 2);
                os.Put( out, 2 );
            }
            // verify termination character
            return (is.Peek() == termChar);
        }
        else
        {
            if (avail == 3)
            {
                avail = 0;
                // decode three bytes
                out[0] = ( data[0]         << 2) + (data[1] >> 4);
                out[1] = ((data[1] & 0x0f) << 4) + (data[2] >> 2);
                out[2] = ((data[2] & 0x03) << 6) +  data[3]      ;
                os.Put( out, 3 );
            }
            else
                avail++;
        }
    }
    return false;
}

std::size_t Base64Decode(unsigned char* dest, unsigned char* src, size_t srcLength)
{
    unsigned char data[4];
    int avail = 0;
    unsigned char* srcEnd = src + srcLength;
    unsigned char* destStart = dest;

    while (src < srcEnd)
    {
        data[avail] = base64Decode[*src++];

        if (data[avail] == base64Ignore)
            continue;
        if (data[avail] == base64Pad)
        {
            if (avail < 2)
                return 0;
            if (avail == 2)
            {
                // verify that next character is a pad as well
                if ((src < srcEnd) && (base64Decode[*src++] != base64Pad))
                    return false;
                // decode one bytes
                *dest++ = (data[0] << 2) + (data[1] >> 4);
            }
            else
            {
                // decode two bytes
                *dest++ = ( data[0]         << 2) + (data[1] >> 4);
                *dest++ = ((data[1] & 0x0f) << 4) + (data[2] >> 2);
            }
            break;
        }
        else
        {
            if (avail == 3)
            {
                avail = 0;
                // decode three bytes
                *dest++ = ( data[0]         << 2) + (data[1] >> 4);
                *dest++ = ((data[1] & 0x0f) << 4) + (data[2] >> 2);
                *dest++ = ((data[2] & 0x03) << 6) +  data[3]      ;
            }
            else
                avail++;
        }
    }
    // verify all data used
    if (src != srcEnd)
        return 0;
    return (dest - destStart);
}

} // namespace internal
} // namespace anyrpc
