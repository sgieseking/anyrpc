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

#ifndef ANYRPC_MESSAGEPACKCLIENT_H_
#define ANYRPC_MESSAGEPACKCLIENT_H_

namespace anyrpc
{

class ANYRPC_API MessagePackHttpClient : public HttpClient
{
public:
    MessagePackHttpClient();
    MessagePackHttpClient(const char* host, int port);
};

////////////////////////////////////////////////////////////////////////////////

class ANYRPC_API MessagePackTcpClient : public TcpClient
{
public:
    MessagePackTcpClient();
    MessagePackTcpClient(const char* host, int port);
};

////////////////////////////////////////////////////////////////////////////////

//! ClientHandler for MessagePack format to generate the request and process the response
class ANYRPC_API MessagePackClientHandler : public ClientHandler
{
public:
    MessagePackClientHandler() {}
    virtual bool GenerateRequest(const char* method, Value& params, Stream& os, unsigned& requestId, bool notification);
    virtual ProcessResponseEnum ProcessResponse(char* response, int length, Value& result, unsigned requestId, bool notification);
};

} // namespace anyrpc

#endif // ANYRPC_MESSAGEPACKCLIENT_H_
