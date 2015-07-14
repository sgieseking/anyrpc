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
    JsonReader reader(is);
    Document doc;
    reader >> doc;

    WriteStringStream os;
    JsonWriter writer(os,ASCII);
    writer << doc.GetValue();

    return os.GetString();
}

static void WriteReadValue(Value& value, Value& outValue)
{
    WriteStringStream os;
    JsonWriter writer(os);
    writer << value;

    //cout << "Json: " << os.GetBuffer() << endl;

    ReadStringStream is(os.GetBuffer());
    JsonReader reader(is);
    Document doc;
    reader >> doc;
    outValue.Assign(doc.GetValue());
}

static string RemoveWhiteSpace(const char* inString, size_t length)
{
    string outString;
    bool insideString = false;

    while (length > 0)
    {
        if (insideString)
        {
            outString += *inString;
            insideString = (*inString != '\"');
        }
        else
        {
            if ((*inString != ' ') &&
                (*inString != '\n') &&
                (*inString != '\r') &&
                (*inString != '\t'))
            {
                outString += *inString;
                insideString = (*inString == '\"');
            }
        }
        inString++;
        length--;
    }

    return outString;
}

static string ParseData(char* filename, string& inString)
{
    FILE* fp = fopen(filename, "r");
    char buffer[65536];
    int len = fread(buffer,1,65535,fp);
    fclose(fp);

    buffer[len] = 0;
    inString = RemoveWhiteSpace(buffer,len);

    ReadFileStream is(filename);
    JsonReader reader(is);
    Document doc;
    reader >> doc;

    WriteStringStream os;
    JsonWriter writer(os);
    writer << doc.GetValue();

    return os.GetString();
}

static int CheckParseError(const char* inString)
{
    ReadStringStream is(inString);
    JsonReader reader(is);
    Document doc;
    reader >> doc;
    return reader.GetParseErrorCode();
}

TEST(Json,Number)
{
    char inString[] = "5736298";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Json,String)
{
    char inString[] = "\"Test string data\"";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Json,Unicode)
{
    char inString[] = "\"\\uD83D\\uDE02\"";
    string outString = ReadWriteData(inString);
    EXPECT_STRCASEEQ( outString.c_str(), inString);
    char inString2[] = "\"\\u0800\\u0080\\uffff\\u1000\\u07ff\\u0fff\\u2452\"";
    string outString2 = ReadWriteData(inString2);
    EXPECT_STRCASEEQ( outString2.c_str(), inString2);
}

TEST(Json,Array)
{
    char inString[] = "[0,1,2,3,4]";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Json,Map)
{
    char inString[] = "{\"item1\":57,\"item2\":89,\"item3\":3.45}";
    string outString = ReadWriteData(inString);
    EXPECT_STREQ( outString.c_str(), inString);
}

TEST(Json,DateTime)
{
    Value value;
    time_t dt = time(NULL);
    value.SetDateTime(dt);
    Value outValue;
	WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsDateTime());
    EXPECT_EQ(outValue.GetDateTime(), value.GetDateTime());
}

TEST(Json,Binary)
{
    Value value;
    char* binData = (char*)"\x0a\x0b\x0c\x0d\xff\x00\xee\xdd\x00";
    value.SetBinary( (unsigned char*)binData, 8);
    Value outValue;
    WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsBinary());
    EXPECT_EQ( strncmp((char*)outValue.GetBinary(), (char*)value.GetBinary(), 8), 0);
}

TEST(Json,SampleGlossary)
{
    char filename[] = "sample/glossary.json";
    string inString;
    string outString = ParseData(filename,inString);
    EXPECT_STREQ( outString.c_str(), inString.c_str());
}

TEST(Json,SampleWebApp)
{
    char filename[] = "sample/webapp.json";
    string inString;
    string outString = ParseData(filename,inString);
    EXPECT_STREQ( outString.c_str(), inString.c_str());
}

TEST(Json,ParseError1)
{
    char inString[] = "[0,1,2,3;4]";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorArrayMissCommaOrSquareBracket);
}

TEST(Json,ParseError2)
{
    char inString[] = "[0,1,2,3,4";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorArrayMissCommaOrSquareBracket);
}

TEST(Json,ParseError3)
{
    char inString[] = "{\"item1\":57,\"item2\":89,\"item3\"}";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorObjectMissColon);
}

TEST(Json,ParseError4)
{
    char inString[] = "7.423e";
    EXPECT_EQ(CheckParseError(inString), AnyRpcErrorNumberMissExponent);
}
