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
#include <fstream>

using namespace std;
using namespace anyrpc;

static string ReadWriteData(char* inString)
{
    ReadStringStream is(inString);
    XmlReader reader(is);
    Document doc;
    reader >> doc;

    WriteStringStream os;
    XmlWriter writer(os);
    writer << doc.GetValue();

    return os.GetString();
}

static void WriteReadValue(Value& value, Value& outValue)
{
    WriteStringStream os;
    XmlWriter writer(os);
    writer << value;

    //cout << "Xml: " << os.GetBuffer() << endl;

    ReadStringStream is(os.GetBuffer());
    XmlReader reader(is);
    Document doc;
    reader >> doc;
    outValue.Assign(doc.GetValue());
}

static int CheckParseError(const char* inString)
{
    ReadStringStream is(inString);
    XmlReader reader(is);
    Document doc;
    reader >> doc;
    return reader.GetParseErrorCode();
}

TEST(Xml,Number)
{
    char inString[] = "<value><i4>5736298</i4></value>";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Xml,Double)
{
    Value value, outValue;

    value.SetDouble(0);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(5);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(2.2348282);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(-728329);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(5.12393e-5);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(-7.192939e-300);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(8e-315);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(-9.12e50);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(1.642e300);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());

    value.SetDouble(-9.999e307);
    WriteReadValue(value, outValue);
    EXPECT_DOUBLE_EQ(value.GetDouble(), outValue.GetDouble());
}

TEST(Xml,String)
{
    char inString[] = "<value>Test string data</value>";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);

    char inString2[] = "<value><string>Test string data</string></value>";
    string outString2 = ReadWriteData(inString2);
    EXPECT_STREQ( outString2.c_str(), inString);
}

TEST(Xml,Array)
{
    char inString[] = "<value><array><data>"
                          "<value><i4>1</i4></value>"
                          "<value><i4>2</i4></value>"
                          "<value><i4>3</i4></value>"
                          "<value><i4>4</i4></value>"
                      "</data></array></value>";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Xml,Map)
{
    char inString[] = "<value><struct>"
                        "<member><name>item1</name><value><i4>57</i4></value></member>"
                        "<member><name>item2</name><value><i4>89</i4></value></member>"
                        "<member><name>item3</name><value><i4>45</i4></value></member>"
                      "</struct></value>";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Xml,DateTime)
{
    Value value;
    time_t dt = time(NULL);
    value.SetDateTime(dt);
    Value outValue;
    WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsDateTime());
    EXPECT_EQ(outValue.GetDateTime(), value.GetDateTime());
}

TEST(Xml,Binary)
{
    Value value;
    char* binData = (char*)"\x0a\x0b\x0c\x0d\xff\x00\xee\xdd\x00";
    value.SetBinary( (unsigned char*)binData, 8);
    Value outValue;
    WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsBinary());
    EXPECT_EQ( strncmp((char*)outValue.GetBinary(), (char*)value.GetBinary(), 8), 0);
}

TEST(Xml,ParseError1)
{
    char inString[] = "<value>Test string data</string></value>";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorTagInvalid);
}

TEST(Xml,ParseError2)
{
    char inString[] = "<value><i4>5736298</value>";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorTagInvalid);
}

TEST(Xml,ParseError3)
{
    char inString[] = "<value><i4>5736298<i4></value>";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorTagInvalid);
}

