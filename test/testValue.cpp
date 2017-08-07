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

using namespace anyrpc;

log_define("AnyRPC.Test");

TEST(Value, Constructors)
{
    Value valueInvalid;
    EXPECT_TRUE(valueInvalid.IsInvalid());

    Value valueTrue(true);
    Value valueFalse(false);
    EXPECT_TRUE(valueTrue.IsBool());
    EXPECT_TRUE(valueTrue.GetBool());
    EXPECT_FALSE(valueFalse.GetBool());

    Value valueInt(400), valueUint(500u);
    EXPECT_EQ(valueInt.GetInt(),400);
    EXPECT_EQ(valueInt.GetUint(),400);
    EXPECT_EQ(valueUint.GetUint(),500);

    Value valueFloat(2.7231f);
    EXPECT_FLOAT_EQ(valueFloat.GetFloat(), 2.7231f);

    Value valueDouble(3.14159);
    EXPECT_DOUBLE_EQ(valueDouble.GetDouble(), 3.14159);

    Value valueString("This is a test");
    EXPECT_STREQ(valueString.GetString(), "This is a test");
}

TEST(Value, Array)
{
    Value value;
    Value addValue(3.5);
    value.SetSize(4);
    value[0] = 5;
    value[2] = "string";
    value.PushBack(addValue);

    EXPECT_TRUE(value.IsArray());
    EXPECT_EQ(value.Size(), 5);
    EXPECT_EQ(value[0].GetInt(), 5);
    EXPECT_EQ(value[4].GetDouble(), 3.5);
    EXPECT_FALSE(value[1].IsValid());
}

TEST(Value, Map)
{
    Value value;
    value["one"] = 1;
    value["two"] = 2;

    EXPECT_TRUE(value.IsMap());
    EXPECT_EQ(value["one"].GetInt(), 1);
    EXPECT_FALSE(value.HasMember("three"));
    EXPECT_TRUE(value.HasMember("two"));

    MemberIterator iter = value.MemberBegin();
    EXPECT_STREQ(iter.GetKey().GetString(), "one");
    EXPECT_EQ(iter.GetValue().GetInt(), 1);
    iter++;
    EXPECT_STREQ(iter.GetKey().GetString(), "two");
    EXPECT_EQ(iter.GetValue().GetInt(), 2);
    iter++;
    EXPECT_EQ(iter, value.MemberEnd());
}

TEST(Value, MapCopy)
{
    Value value;
    value["one"] = 1;
    value["two"] = 2;

    Value value2;
    // perform copy - value will still be a map
    value2 = value;
    EXPECT_TRUE(value.IsMap());
    EXPECT_TRUE(value2.IsMap());

    // perform assignment again - checking for proper destructor operation
    value2 = value;
    EXPECT_TRUE(value.IsMap());
    EXPECT_TRUE(value2.IsMap());
}

TEST(Value, Assign)
{
    Value value;
    Value value2(5);

    value.Assign(value2);
    EXPECT_TRUE(value.IsValid());
    EXPECT_TRUE(value2.IsNull());
    EXPECT_EQ(value.GetInt(), 5);
}

TEST(Value, Copy)
{
    Value value;
    Value value2(10);

    value.Copy(value2);
    EXPECT_TRUE(value.IsValid());
    EXPECT_TRUE(value2.IsValid());
    EXPECT_EQ(value.GetInt(), 10);
    EXPECT_EQ(value2.GetInt(), 10);
}

#if defined(ANYRPC_WCHAR)
TEST(Value, Unicode)
{
    const wchar_t *s1 = L"This is a test";
    Value value(s1);
    EXPECT_STREQ(value.GetString(), "This is a test");
    EXPECT_STREQ(value.GetWString().c_str(), s1);

    const wchar_t *s2 = L"\x7f\x80\u07ff\u0800\ufffe\U00010000\U0001FFF0";
    value.SetString(s2);
    EXPECT_STREQ(value.GetString(), "\x7F\xC2\x80\xDF\xBF\xE0\xA0\x80\xEF\xBF\xBE\xF0\x90\x80\x80\xF0\x9F\xBF\xB0");
    EXPECT_STREQ(value.GetWString().c_str(), s2);
}
#endif
