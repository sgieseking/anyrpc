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
#include "anyrpc/internal/time.h"
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
#include "anyrpc/json/jsonwriter.h"
#include "anyrpc/json/jsonreader.h"
#include "anyrpc/json/jsonclient.h"

namespace anyrpc
{

//! Only need one instance of the JsonClientHandler since there is no local storage.
static JsonClientHandler jsonClientHandler;

////////////////////////////////////////////////////////////////////////////////

JsonHttpClient::JsonHttpClient() : HttpClient(&jsonClientHandler, "application/json-rpc") {}

JsonHttpClient::JsonHttpClient(const char* host, int port) :
        HttpClient(&jsonClientHandler, "application/json-rpc", host, port) {}

////////////////////////////////////////////////////////////////////////////////

JsonTcpClient::JsonTcpClient() : TcpClient(&jsonClientHandler) {}

JsonTcpClient::JsonTcpClient(const char* host, int port) :
        TcpClient(&jsonClientHandler, host, port) {}

////////////////////////////////////////////////////////////////////////////////

bool JsonClientHandler::GenerateRequest(const char* method, Value& params, Stream& os, unsigned& requestId, bool notification)
{
    log_trace();
    Value request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;
    request["params"].Assign(params);   // assign so that the structure doesn't need to be copied
    if (!notification)
    {
        requestId = GetNextId();
        request["id"] = requestId;
    }
    else
        requestId = 0;

    JsonWriter jsonStrWriter(os);
    request.Traverse(jsonStrWriter);

    params.Assign(request["params"]);   // move the params back so the user still has access to them

    return true;
}

ProcessResponseEnum JsonClientHandler::ProcessResponse(char* response, size_t length, Value& result, unsigned requestId, bool notification)
{
    log_trace();
    ProcessResponseEnum processResponse = ProcessResponseErrorClose;
    try
    {
        Document doc;

        InSituStringStream sstream(response, length);
        JsonReader reader(sstream);
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

            if (message.IsMap())
            {
                if (!message.HasMember("jsonrpc"))
                    GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, missing jsonrpc member", result);
                else if (!message.HasMember("id"))
                    GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, missing id member", result);
                else
                {
                    Value& id = message["id"];
                    Value& rpc = message["jsonrpc"];

                    if (!rpc.IsString() || (strcmp(rpc.GetString(), "2.0") != 0))
                        GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, rpc version", result);
                    else if (!id.IsUint() || (id.GetUint() != requestId))
                    {
                        log_debug("Invalid id:" << id << ", expected id=" << requestId);
                        GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, bad id", result);
                    }
                    else if (message.HasMember("error"))
                    {
                        result.Assign(message["error"]);
                        if (result.HasMember("code") && result["code"].IsInt())
                        {
                            int code = result["code"].GetInt();
                            // keep the close open if only an application type of error
                            if ((code > AnyRpcErrorTransportError) || (code < AnyRpcErrorApplicationError))
                                processResponse = ProcessResponseErrorKeepOpen;
                        }
                    }
                    else if (!message.HasMember("result"))
                        GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, no result", result);
                    else
                    {
                        result.Assign(message["result"]);
                        processResponse = ProcessResponseSuccess;
                    }
                }
            }
            else if (message.IsInvalid() && notification)
            {
                // response from notification with transport that required a response
                processResponse = ProcessResponseSuccess;
            }
            else
            {
                log_debug("message type = " << message.GetType());
                GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, wrong message type", result);
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
