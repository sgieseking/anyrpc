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

#ifndef ANYRPC_MESSAGEPACKSERVER_H_
#define ANYRPC_MESSAGEPACKSERVER_H_

namespace anyrpc
{

ANYRPC_API bool MessagePackRpcHandler(MethodManager* manager, char* request, int length, Stream &response);

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API MessagePackHttpServer : public ServerST
{
public:
    MessagePackHttpServer() { AddHandler( &MessagePackRpcHandler, "", "application/messagepack-rpc" ); }

protected:
    virtual Connection* CreateConnection(int fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API MessagePackTcpServer : public ServerST
{
protected:
    virtual Connection* CreateConnection(int fd) { return new TcpConnection(fd, GetMethodManager(), &MessagePackRpcHandler); }
};

////////////////////////////////////////////////////////////////////////////////

#if defined(ANYRPC_THREADING)
class ANYRPC_API MessagePackHttpServerMT : public ServerMT
{
public:
    MessagePackHttpServerMT() { AddHandler( &MessagePackRpcHandler, "", "application/messagepack-rpc" ); }

protected:
    virtual Connection* CreateConnection(int fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API MessagePackTcpServerMT : public ServerMT
{
protected:
    virtual Connection* CreateConnection(int fd) { return new TcpConnection(fd, GetMethodManager(), &MessagePackRpcHandler); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API MessagePackHttpServerTP : public ServerTP
{
public:
    MessagePackHttpServerTP() { AddHandler( &MessagePackRpcHandler, "", "application/messagepack-rpc" ); }

protected:
    virtual Connection* CreateConnection(int fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API MessagePackTcpServerTP : public ServerTP
{
protected:
    virtual Connection* CreateConnection(int fd) { return new TcpConnection(fd, GetMethodManager(), &MessagePackRpcHandler); }
};
#endif

} // namespace anyrpc

#endif // ANYRPC_MESSAGEPACKSERVER_H_
