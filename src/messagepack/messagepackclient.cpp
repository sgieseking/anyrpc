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

#include "anyrpc/api.h"
#include "anyrpc/logger.h"
#include "anyrpc/error.h"
#include "anyrpc/value.h"
#include "anyrpc/stream.h"
#include "anyrpc/handler.h"
#include "anyrpc/reader.h"
#include "anyrpc/document.h"
#include "anyrpc/method.h"
#include "anyrpc/socket.h"
#include "anyrpc/client.h"
#include "anyrpc/messagepack/messagepackwriter.h"
#include "anyrpc/messagepack/messagepackreader.h"
#include "anyrpc/messagepack/messagepackclient.h"

namespace anyrpc
{

//! Only need one instance of the MessagePackClientHandler since there is no local storage.
static MessagePackClientHandler mpackClientHandler;

////////////////////////////////////////////////////////////////////////////////

MessagePackHttpClient::MessagePackHttpClient() : HttpClient(&mpackClientHandler, "application/messagepack-rpc") {}

MessagePackHttpClient::MessagePackHttpClient(const char* host, int port) :
        HttpClient(&mpackClientHandler, "application/messagepack-rpc", host, port) {}

////////////////////////////////////////////////////////////////////////////////

MessagePackTcpClient::MessagePackTcpClient() : TcpClient(&mpackClientHandler) {}

MessagePackTcpClient::MessagePackTcpClient(const char* host, int port) :
        TcpClient(&mpackClientHandler, host, port) {}

////////////////////////////////////////////////////////////////////////////////

bool MessagePackClientHandler::GenerateRequest(const char* method, Value& params, Stream& os, unsigned& requestId, bool notification)
{
    log_trace();
    Value request;
    if (notification)
    {
        requestId = 0;
        request.SetSize(3);
        request[0] = 2;
        request[1] = method;
        request[2].Assign(params);      // assign so that the structure doesn't need to be copied
    }
    else
    {
        requestId = GetNextId();
        request.SetSize(4);
        request[0] = 0;
        request[1] = requestId;
        request[2] = method;
        request[3].Assign(params);      // assign so that the structure doesn't need to be copied
    }

    MessagePackWriter mpackStrWriter(os);
    request.Traverse(mpackStrWriter);

    if (notification)                   // move the params back so the user still has access to them
        params.Assign(request[2]);
    else
        params.Assign(request[3]);
    return true;
}

ProcessResponseEnum MessagePackClientHandler::ProcessResponse(char* response, int length, Value& result, unsigned requestId, bool notification)
{
    log_trace();
    ProcessResponseEnum processResponse = ProcessResponseErrorClose;
    try
    {
        Document doc;

        log_debug("Response length=" << length << ", requestId=" << requestId);

        if (notification && (length == 0))
            return processResponse;

        InSituStringStream sstream(response, length);
        MessagePackReader reader(sstream);
        reader.ParseStream(doc);
        if (reader.HasParseError())
        {
            std::stringstream message;
            message << "Response parse error, offset=" << reader.GetErrorOffset();
            message << ", code=" << reader.GetParseErrorCode();
            message << ", message=" << reader.GetParseErrorStr();
            GenerateFaultResult(AnyRpcErrorResponseParseError, message.str(), result);
        }
        else
        {
            Value message;
            message.Assign( doc.GetValue() );
            log_debug( "Parsed: " << message );

            if (message.IsArray() && (message.Size() == 4))
            {
                Value& type = message[0];
                Value& id = message[1];
                Value& fault = message[2];
                Value& msgResult = message[3];

                if (!type.IsInt() || (type.GetInt() != 1))
                    GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, wrong type", result);
                else if (id.IsInvalid() || (id.GetUint() != requestId))
                {
                    log_debug("Invalid id:" << id << ", expected id=" << requestId);
                    GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, bad id", result);
                }
                else if (!fault.IsNull())
                {
                    result.Assign(fault);
                    if (result.HasMember("code") && result.HasMember("message"))
                        // standard fault response - keep the connection open
                        processResponse =  ProcessResponseErrorKeepOpen;
                    else
                    {
                        // response doesn't have the correct fields
                        // technically not require by the specification by expected from the server
                        GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, wrong fault fields", result);
                    }
                }
                else
                {
                    result.Assign(msgResult);
                    processResponse = ProcessResponseSuccess;
                }
            }
            else
            {
                log_debug("message type = " << message.GetType() << ", Size = " << message.Size());
                GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, response not an array of length 4", result);
            }
        }
    }
    catch (AnyRpcException &fault)
    {
        std::stringstream buffer;
        buffer << "Unhandled system error, Code:" << fault.GetCode() << ", Message:" << fault.GetMessage();
        GenerateFaultResult(AnyRpcErrorSystemError, buffer.str(), result);
    }

    return processResponse;
}

} // namespace anyrpc
