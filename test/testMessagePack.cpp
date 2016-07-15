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

static void WriteReadValue(Value& value, Value& outValue)
{
    // Use file streams since binary data may contain embedded NULLs
    WriteFileStream os("test.bin");
    MessagePackWriter writer(os);
    writer << value;
    os.Close();

    ReadFileStream is("test.bin");
    MessagePackReader reader(is);
    Document doc;
    reader >> doc;
    outValue.Assign(doc.GetValue());
    EXPECT_FALSE(reader.HasParseError());
}

static void WriteReadValueInsitu(Value& value, Value& outValue, WriteStringStream& wstream)
{
    // Use file streams since binary data may contain embedded NULLs
    MessagePackWriter writer(wstream);
    writer << value;

    const char* mPack = wstream.GetBuffer();
    size_t mPacklength = wstream.Length();
    InSituStringStream rstream((char*)mPack, mPacklength);
    MessagePackReader reader(rstream);
    Document doc;
    reader >> doc;

    outValue.Assign(doc.GetValue());
    EXPECT_FALSE(reader.HasParseError());
}

TEST(MessagePack,Number)
{
    Value value;
    Value outValue;

    value.SetInt(192);
    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsInt());
    EXPECT_EQ(outValue.GetInt(), value.GetInt());

    value.SetInt(567);
    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsInt());
    EXPECT_EQ(outValue.GetInt(), value.GetInt());

    value.SetFloat(5.22392f);
    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsFloat());
    EXPECT_FLOAT_EQ( (float)outValue.GetDouble(), (float)value.GetDouble());

    value.SetDouble(7.2309345679);
    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsDouble());
    EXPECT_DOUBLE_EQ(outValue.GetDouble(), value.GetDouble());
}

TEST(MessagePack,String)
{
    Value value;
    value.SetString("Test string data");
    Value outValue;

    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsString());
    EXPECT_STREQ(outValue.GetString(), value.GetString());

    value.SetString("0123456789012345678901234567890123456789012345678901234567890123456789"
                    "0123456789012345678901234567890123456789012345678901234567890123456789");
    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsString());
    EXPECT_STREQ(outValue.GetString(), value.GetString());

    value.SetString("0123456789012345678901234567890123456789012345678901234567890123456789"
                    "0123456789012345678901234567890123456789012345678901234567890123456789"
                    "0123456789012345678901234567890123456789012345678901234567890123456789"
                    "0123456789012345678901234567890123456789012345678901234567890123456789");
    WriteReadValue(value, outValue);
    EXPECT_TRUE(outValue.IsString());
    EXPECT_STREQ(outValue.GetString(), value.GetString());
}

TEST(MessagePack,Array)
{
    Value value;
    value[0] = 47;
    value[1] = 63;
    value[2] = 87.321;
    Value outValue;
    WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsArray());
    EXPECT_EQ(outValue[0].GetInt(), value[0].GetInt());
    EXPECT_EQ(outValue[1].GetInt(), value[1].GetInt());
    EXPECT_DOUBLE_EQ(outValue[2].GetDouble(), value[2].GetDouble());
}

TEST(MessagePack,Map)
{
    Value value;
    value["item1"] = 47;
    value["item2"] = 0;
    value["item3"] = 87.321;
    Value outValue;

    // the stream must be passed in so that the string is still valid with insitu parsing
    WriteStringStream wstream;
    WriteReadValueInsitu(value, outValue, wstream);

    EXPECT_TRUE(outValue.IsMap());
    EXPECT_EQ(outValue["item1"].GetInt(), value["item1"].GetInt());
    EXPECT_EQ(outValue["item2"].GetInt(), value["item2"].GetInt());
    EXPECT_DOUBLE_EQ(outValue["item3"].GetDouble(), value["item3"].GetDouble());
}

TEST(MessagePack,DateTime)
{
    Value value;
    time_t dt = time(NULL);
    value.SetDateTime(dt);
    Value outValue;
    WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsDateTime());
    EXPECT_EQ(outValue.GetDateTime(), value.GetDateTime());
}

TEST(MessagePack,Binary)
{
    Value value;
    char* binData = (char*)"\x0a\x0b\x0c\x0d\xff\x00\xee\xdd\x00";
    value.SetBinary( (unsigned char*)binData, 8);
    Value outValue;
    WriteReadValue(value, outValue);

    EXPECT_TRUE(outValue.IsBinary());
    EXPECT_EQ( strncmp((char*)outValue.GetBinary(), (char*)value.GetBinary(), 8), 0);
}
