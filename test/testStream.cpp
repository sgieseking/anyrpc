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

#include "anyrpc/anyrpc.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace std;
using namespace anyrpc;

static string abcString = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

TEST(Stream,WriteSegmentedStream)
{
    WriteSegmentedStream wstream;
    string inString;
    for (int i=0; i<1000; i++)
    {
        wstream.Put(abcString);
        inString += abcString;
    }

    string outString;
    size_t offset = 0;
    size_t length;
    while (offset < wstream.Length())
    {
        outString += wstream.GetBuffer(offset, length);
        offset += length;
    }

    EXPECT_STREQ(outString.c_str(), inString.c_str());
}

TEST(Stream,WriteStringStream)
{
    WriteStringStream wstream;
    string inString;
    for (int i=0; i<1000; i++)
    {
        wstream.Put(abcString);
        inString += abcString;
    }

    const string& outString = wstream.GetString();

    EXPECT_STREQ(outString.c_str(), inString.c_str());
}

TEST(Stream,FileStream)
{
    char binFile[] = "test.bin";

    WriteFileStream wfstream(binFile);

    for (int i=0; i<100; i++)
    {
        wfstream.Put(abcString);
    }

    wfstream.Close();

    ReadFileStream rfstream(binFile);

    char inbuf[30];
    for (int i=0; i<100; i++)
    {
        size_t bytesRead = rfstream.Read(inbuf,26);
        inbuf[bytesRead] = 0;
        EXPECT_EQ(bytesRead, 26);
        EXPECT_STREQ(inbuf, abcString.c_str());
    }

    EXPECT_TRUE(rfstream.Eof());
}

TEST(Stream,ReadStringStream)
{
    const char* inString = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    ReadStringStream stream(inString);

    char inbuf[50];
    stream.Read(inbuf,5);
    inbuf[5] = 0;
    EXPECT_STREQ(inbuf, "ABCDE");
    EXPECT_EQ( stream.Peek(), 'F');
    EXPECT_EQ( stream.Get(), 'F');
    EXPECT_FALSE( stream.Eof() );
}

TEST(Stream,InSituStringStream)
{
    const char* baseString = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char inString[50];

    strcpy(inString,baseString);

    InSituStringStream stream(inString, strlen(inString));

    char inbuf[50];
    stream.Read(inbuf,5);
    inbuf[5] = 0;
    EXPECT_STREQ(inbuf, "ABCDE");
    EXPECT_EQ( stream.Peek(), 'F');
    EXPECT_EQ( stream.Get(), 'F');
    EXPECT_FALSE( stream.Eof() );
    stream.PutBegin();
    stream.Read(inbuf,10);
    inbuf[10] = 0;
    stream.Put(inbuf+5,5);
    stream.PutEnd();

    EXPECT_STREQ( inString, "ABCDEFLMNOPLMNOPQRSTUVWXYZ");

    stream.Read(inbuf,5);
    inbuf[5] = 0;
    EXPECT_STREQ(inbuf, "QRSTU");
}
