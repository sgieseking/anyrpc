#include "anyrpc/anyrpc.h"

#include <gtest/gtest.h>

using namespace std;
using namespace anyrpc;

void Add(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 2) ||
        (!params[0].IsNumber()) ||
        (!params[1].IsNumber()))
        return;
    result = params[0].GetDouble() + params[1].GetDouble();
}

void Subtract(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 2) ||
        (!params[0].IsNumber()) ||
        (!params[1].IsNumber()))
        return;
    result = params[0].GetDouble() - params[1].GetDouble();
}

class Multiply : public Method
{
public:
    Multiply() :
        Method("multiply", "Multiply two numbers",false) {}
    virtual void Execute(Value& params, Value& result)
    {
        if ((!params.IsArray()) ||
            (params.Size() != 2) ||
            (!params[0].IsNumber()) ||
            (!params[1].IsNumber()))
            return;
        result = params[0].GetDouble() * params[1].GetDouble();
    }
};

TEST(MethodMap,General)
{
    Multiply multiply;

    MethodManager methodManager;

    methodManager.AddFunction( &Add, "add", "Add two numbers");
    methodManager.AddFunction( &Subtract, "subtract", "Subtract two numbers");
    methodManager.AddMethod( &multiply );

    Value params;
    Value result;
    params.SetArray(2);
    params[0] = 5;
    params[1] = 3;

    methodManager.ExecuteMethod("add",params,result);
    EXPECT_DOUBLE_EQ(result.GetDouble(), 8);

    methodManager.ExecuteMethod("subtract",params,result);
    EXPECT_DOUBLE_EQ(result.GetDouble(), 2);

    methodManager.ExecuteMethod("multiply",params,result);
    EXPECT_DOUBLE_EQ(result.GetDouble(), 15);

    params.SetNull();
    methodManager.ExecuteMethod(LIST_METHODS,params,result);
    EXPECT_STREQ(result[0].GetString(), "add");
    EXPECT_STREQ(result[1].GetString(), "multiply");
    EXPECT_STREQ(result[2].GetString(), "subtract");
    EXPECT_STREQ(result[3].GetString(), "system.listMethods");
    EXPECT_STREQ(result[4].GetString(), "system.methodHelp");

    params.SetArray();
    params[0] = "add";
    methodManager.ExecuteMethod(METHOD_HELP,params,result);
    EXPECT_STREQ(result.GetString(),"Add two numbers");
}
