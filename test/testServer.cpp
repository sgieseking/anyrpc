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
#include "anyrpc/internal/time.h"

#include <gtest/gtest.h>

using namespace std;
using namespace anyrpc;

#if defined(ANYRPC_THREADING)

static const int ServerPort = 9000;
static const char* ServerIpAddress = "127.0.0.1";

static string abcString = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

log_define("root");

// Define the method functions that can be called
static void Add(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 2) ||
        (!params[0].IsNumber()) ||
        (!params[1].IsNumber()))
        throw AnyRpcException(AnyRpcErrorInvalidParams, "Invalid parameters");
    result = params[0].GetDouble() + params[1].GetDouble();
}

static void Subtract(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 2) ||
        (!params[0].IsNumber()) ||
        (!params[1].IsNumber()))
        throw AnyRpcException(AnyRpcErrorInvalidParams, "Invalid parameters");
    result = params[0].GetDouble() - params[1].GetDouble();
}

static void Echo(Value& params, Value& result)
{
    result = params;
}

static void ServerSetup(Server& server)
{
    server.BindAndListen(ServerPort);

    // Add the method calls
    MethodManager *methodManager = server.GetMethodManager();
    methodManager->AddFunction( &Add, "add", "Add two numbers");
    methodManager->AddFunction( &Subtract, "subtract", "Subtract two numbers");
    methodManager->AddFunction( &Echo, "echo", "Return the same data that was sent");
}

static void TestClient(Client &client)
{
    Value params;
    Value result;
    bool success;

    MilliSleep(50);
    client.SetServer(ServerIpAddress, ServerPort);
    client.SetTimeout(2000);

    // Add the parameters
    // Note that the parameters will be invalidated inside the Call
    params.SetArray();
    params[0] = 5;
    params[1] = 6;
    success = client.Call("add", params, result);
    EXPECT_TRUE(success);
    if (success)
    {
		if (result.IsInt())
			EXPECT_EQ(result.GetInt(), 11);
		else
			EXPECT_DOUBLE_EQ(result.GetDouble(), 11);
    }

    // Subtract the parameters
    success = client.Call("subtract", params, result);
    EXPECT_TRUE(success);
    if (success)
    {
		if (result.IsInt())
			EXPECT_EQ(result.GetInt(), -1);
		else
			EXPECT_DOUBLE_EQ(result.GetDouble(), -1);
    }

    // Call divide but this is not implement so expect a failure
    success = client.Call("divide", params, result);
    EXPECT_FALSE(success);

    // Send large data set
    const int numValues = 100;
    params.SetSize(numValues);
    for (int i=0; i<numValues; i++)
        params[i] = abcString;
    success = client.Call("echo", params, result);
    EXPECT_TRUE(success);
    if (success)
    {
		ASSERT_TRUE(result.IsArray());
		for (int i=0; i<numValues; i++)
		{
			ASSERT_TRUE(result[i].IsString());
			ASSERT_STREQ(result[i].GetString(), abcString.c_str());
		}
    }
}

#if defined(ANYRPC_INCLUDE_JSON)
TEST(Server, JsonHttp)
{
	log_time(WARN,"JsonHttp");
    JsonHttpServer server;
    JsonHttpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, JsonHttpMultiple)
{
    log_time(WARN,"JsonHttpMultiple");
    JsonHttpServer server;
    JsonHttpClient client[4];

    server.SetMaxConnections(2);
    ServerSetup(server);
    server.StartThread();
    for (int i=0; i<4; i++)
        TestClient(client[i]);
    server.StopThread();
}

TEST(Server, JsonTcp)
{
	log_time(WARN, "JsonTcp");
    JsonTcpServer server;
    JsonTcpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, JsonHttpMT)
{
	log_time(WARN, "JsonHttpMT");
    JsonHttpServerMT server;
    JsonHttpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, JsonHttpMTMultiple)
{
    log_time(WARN,"JsonHttpMTMultiple");
    JsonHttpServerMT server;
    JsonHttpClient client[4];

    server.SetMaxConnections(2);
    ServerSetup(server);
    server.StartThread();
    for (int i=0; i<4; i++)
        TestClient(client[i]);
    server.StopThread();
}

TEST(Server, JsonTcpMT)
{
	log_time(WARN, "JsonTcpMT");
    JsonTcpServerMT server;
    JsonTcpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}
#endif // defined(ANYRPC_INCLUDE_JSON)
#if defined(ANYRPC_INCLUDE_XML)
TEST(Server, XmlHttp)
{
	log_time(WARN, "XmlHttp");
    XmlHttpServer server;
    XmlHttpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, XmlTcp)
{
	log_time(WARN, "XmlTcp");
    XmlTcpServer server;
    XmlTcpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, XmlHttpMT)
{
	log_time(WARN, "XmlHttpMT");
    XmlHttpServerMT server;
    XmlHttpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, XmlTcpMT)
{
	log_time(WARN, "XmlTcpMT");
    XmlTcpServerMT server;
    XmlTcpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}
#endif // defined(ANYRPC_INCLUDE_XML)
#if defined(ANYRPC_INCLUDE_MESSAGEPACK)
TEST(Server, MessagePackHttp)
{
	log_time(WARN, "MessagePackHttp");
    MessagePackHttpServer server;
    MessagePackHttpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, MessagePackTcp)
{
	log_time(WARN, "MessagePackTcp");
    MessagePackTcpServer server;
    MessagePackTcpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, MessagePackHttpMT)
{
	log_time(WARN, "MessagePackHttpMT");
    MessagePackHttpServerMT server;
    MessagePackHttpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}

TEST(Server, MessagePackTcpMT)
{
	log_time(WARN, "MessagePackTcpMT");
    MessagePackTcpServerMT server;
    MessagePackTcpClient client;

    ServerSetup(server);
    server.StartThread();
    TestClient(client);
    server.StopThread();
}
#endif // defined(ANYRPC_INCLUDE_MESSAGEPACK)
#endif // defined(ANYRPC_THREADING)
