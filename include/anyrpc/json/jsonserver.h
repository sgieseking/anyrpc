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

#ifndef ANYRPC_JSONSERVER_H_
#define ANYRPC_JSONSERVER_H_

namespace anyrpc
{

ANYRPC_API bool JsonRpcHandler(MethodManager* manager, char* request, std::size_t length, Stream &response);

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API JsonHttpServer : public ServerST
{
public:
    JsonHttpServer() { AddHandler( &JsonRpcHandler, "", "application/json-rpc" ); }

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API JsonTcpServer : public ServerST
{
protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new TcpConnection(fd, GetMethodManager(), &JsonRpcHandler); }
};

////////////////////////////////////////////////////////////////////////////////

#if defined(ANYRPC_THREADING)
class ANYRPC_API JsonHttpServerMT : public ServerMT
{
public:
    JsonHttpServerMT() { AddHandler( &JsonRpcHandler, "", "application/json-rpc" ); }

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API JsonTcpServerMT : public ServerMT
{
protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new TcpConnection(fd, GetMethodManager(), &JsonRpcHandler); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API JsonHttpServerTP : public ServerTP
{
public:
    JsonHttpServerTP() { AddHandler( &JsonRpcHandler, "", "application/json-rpc" ); }
    JsonHttpServerTP(const unsigned numThreads) : ServerTP(numThreads) { AddHandler( &JsonRpcHandler, "", "application/json-rpc" ); }

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API JsonTcpServerTP : public ServerTP
{
public:
    JsonTcpServerTP() : ServerTP() {};
    JsonTcpServerTP(const unsigned numThreads) : ServerTP(numThreads) {};

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new TcpConnection(fd, GetMethodManager(), &JsonRpcHandler); }
};
#endif

} // namespace anyrpc

#endif // ANYRPC_JSONSERVER_H_
