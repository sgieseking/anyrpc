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
#include "anyrpc/internal/http.h"

#include <gtest/gtest.h>
#include <fstream>

using namespace std;
using namespace anyrpc;
using namespace anyrpc::internal;

TEST(HttpHeader,RequestSingleRead)
{
    const char* inString =  "POST /RPC2 HTTP/1.1\r\n"
                            " Host: 192.168.1.1:5000\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            "\r\n"
                            "body";

    HttpRequest request;
    bool eof=false;
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.1");
    EXPECT_STREQ(request.GetHost().c_str(), "192.168.1.1:5000");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_TRUE(request.GetKeepAlive());
}

TEST(HttpHeader,RequestMultipleRead)
{
    const char* inString =  "POST /RPC2 HTTP/1.1\r\n"
                            " Host: 192.168.1.1:5000\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            "\r\n";

    HttpRequest request;
    bool eof=false;
    for (size_t i=0; i<strlen(inString); i++)
        EXPECT_EQ(request.ProcessHeaderData(inString, i, eof), HttpHeader::HEADER_INCOMPLETE);
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.1");
    EXPECT_STREQ(request.GetHost().c_str(), "192.168.1.1:5000");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_TRUE(request.GetKeepAlive());
}

TEST(HttpHeader,RequestOnlyLF)
{
    const char* inString =  "POST /RPC2 HTTP/1.1\n"
                            " Host: 192.168.1.1:5000\n"
                            " Content-length: 47\n"
                            " Content-type: text/xml\n"
                            "\n"
                            "body";

    HttpRequest request;
    bool eof=false;
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.1");
    EXPECT_STREQ(request.GetHost().c_str(), "192.168.1.1:5000");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_TRUE(request.GetKeepAlive());
}

TEST(HttpHeader,RequestHttp11MissingHost)
{
    const char* inString =  "POST /RPC2 HTTP/1.1\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            "\r\n"
                            "body";

    HttpRequest request;
    bool eof=false;
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_FAULT);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.1");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_TRUE(request.GetKeepAlive());
}

TEST(HttpHeader,RequestHttp10MissingHost)
{
    const char* inString =  "POST /RPC2 HTTP/1.0\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            "\r\n"
                            "body";

    HttpRequest request;
    bool eof=false;
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.0");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_FALSE(request.GetKeepAlive());
}

TEST(HttpHeader,RequestHttp10KeepAlive)
{
    const char* inString =  "POST /RPC2 HTTP/1.0\r\n"
                            " Host: 192.168.1.1:5000\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            " Connection: keep-alive\r\n"
                            "\r\n"
                            "body";

    HttpRequest request;
    bool eof=false;
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.0");
    EXPECT_STREQ(request.GetHost().c_str(), "192.168.1.1:5000");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_TRUE(request.GetKeepAlive());
}

TEST(HttpHeader,RequestHttp11Close)
{
    const char* inString =  "POST /RPC2 HTTP/1.1\r\n"
                            " Host: 192.168.1.1:5000\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            " Connection: close\r\n"
                            "\r\n"
                            "body";

    HttpRequest request;
    bool eof=false;
    EXPECT_EQ(request.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(request.GetMethod().c_str(), "POST");
    EXPECT_STREQ(request.GetRequestUri().c_str(), "/RPC2");
    EXPECT_STREQ(request.GetHttpVersion().c_str(), "1.1");
    EXPECT_STREQ(request.GetHost().c_str(), "192.168.1.1:5000");
    EXPECT_EQ(request.GetContentLength(), 47);
    EXPECT_FALSE(request.GetKeepAlive());
}

TEST(HttpHeader,ResponseSingleRead)
{
    const char* inString =  "HTTP/1.1 200 OK\r\n"
                            " Server: AnyRPC\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            "\r\n"
                            "body";

    HttpResponse response;
    bool eof=false;
    EXPECT_EQ(response.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(response.GetHttpVersion().c_str(), "1.1");
    EXPECT_STREQ(response.GetResponseCode().c_str(), "200");
    EXPECT_STREQ(response.GetResponseString().c_str(), "OK");
    EXPECT_EQ(response.GetContentLength(), 47);
    EXPECT_TRUE(response.GetKeepAlive());
}

TEST(HttpHeader,ResponseMultipleRead)
{
    const char* inString =  "HTTP/1.1 200 OK\r\n"
                            " Server: AnyRPC\r\n"
                            " Content-length: 47\r\n"
                            " Content-type: text/xml\r\n"
                            "\r\n";

    HttpResponse response;
    bool eof=false;
    for (size_t i=0; i<strlen(inString); i++)
        EXPECT_EQ(response.ProcessHeaderData(inString, i, eof), HttpHeader::HEADER_INCOMPLETE);
    EXPECT_EQ(response.ProcessHeaderData(inString, strlen(inString), eof), HttpHeader::HEADER_COMPLETE);
    EXPECT_STREQ(response.GetHttpVersion().c_str(), "1.1");
    EXPECT_STREQ(response.GetResponseCode().c_str(), "200");
    EXPECT_STREQ(response.GetResponseString().c_str(), "OK");
    EXPECT_EQ(response.GetContentLength(), 47);
    EXPECT_TRUE(response.GetKeepAlive());
}

