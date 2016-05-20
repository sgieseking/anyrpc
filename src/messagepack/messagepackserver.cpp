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
#include "anyrpc/connection.h"
#include "anyrpc/server.h"
#include "anyrpc/messagepack/messagepackwriter.h"
#include "anyrpc/messagepack/messagepackreader.h"
#include "anyrpc/messagepack/messagepackserver.h"

namespace anyrpc
{

static void MessagePackGenerateResponse(Value& result, Value& id, Value& response);
static void MessagePackGenerateFaultResponse(int errorCode, std::string const& errorMsg, Value& id, Value& response);

log_define("AnyRPC.RpcHandler");

////////////////////////////////////////////////////////////////////////////////

bool MessagePackRpcHandler(MethodManager* manager, char* request, size_t length, Stream &response)
{
    log_trace();
    Document doc;
    Value valueResponse;
    Value nullValue;
    nullValue.SetNull();
    bool notification = false;

    InSituStringStream sstream(request, length);
    MessagePackReader reader(sstream);
    reader.ParseStream(doc);
    if (reader.HasParseError())
        MessagePackGenerateFaultResponse(AnyRpcErrorParseError, "Parse error", nullValue, valueResponse);
    else
    {
        Value message;
        message.Assign( doc.GetValue() );

        if (message.IsArray() && (message.Size() == 4))
        {
            // process method call
            Value& type = message[0];
            Value& id = message[1];
            Value& method = message[2];
            Value& params = message[3];

            if (!type.IsInt() && (type.GetInt() != 0))
                MessagePackGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
            else if (!id.IsUint())
                MessagePackGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
            else if (!method.IsString())
                MessagePackGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
            else
            {
                Value result;
                result.SetNull();

                std::string methodName = method.GetString();
                try
                {
                    if (!manager->ExecuteMethod(methodName, params, result))
                        MessagePackGenerateFaultResponse(AnyRpcErrorMethodNotFound, "Method not found", id, valueResponse);
                    else
                        MessagePackGenerateResponse(result, id, valueResponse);
                }
                catch (const AnyRpcException& fault)
                {
                    MessagePackGenerateFaultResponse(fault.GetCode(), fault.GetMessage(), id, valueResponse);
                }
            }
        }
        else if (message.IsArray() && (message.Size() == 3))
        {
            // process notification
            notification = true;

            // Should a notification produce a fault response if it is not formatted correctly?
			// The protocol specification indicates no response should be given to a notification.
            Value& type = message[0];
            Value& method = message[2];
            Value& params = message[3];

            if (!type.IsInt() && (type.GetInt() != 1))
            {
                //GenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
            }
            else if (!method.IsString())
            {
                //GenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
            }
            else
            {
                Value result;
                result.SetNull();
                Value id;
                id.SetNull();

                std::string methodName = method.GetString();
                try
                {
                    manager->ExecuteMethod(methodName, params, result);
                    //if (!manager->ExecuteMethod(methodName, params, result))
                    //    GenerateFaultResponse(AnyRpcErrorMethodNotFound, "Method not found", id, valueResponse);
                }
                catch (const AnyRpcException&)
                {
                    //GenerateFaultResponse(fault.GetCode(), fault.GetMessage(), id, valueResponse);
                }
            }
        }
        else
        {
            log_debug("message type = " << message.GetType() << ", Size = " << message.Size());
            MessagePackGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
        }
    }

    if (notification)
        // notification doesn't produce a response message
        return false;

    MessagePackWriter mpackStrWriter(response);
    valueResponse.Traverse(mpackStrWriter);

    return true;
}

static void MessagePackGenerateResponse(Value& result, Value& id, Value& response)
{
    response.SetSize(4);
    response[0] = 1;
    response[1] = id;
    response[2].SetNull();
    response[3].Assign(result);
}

static void MessagePackGenerateFaultResponse(int errorCode, std::string const& errorMsg, Value& id, Value& response)
{
    response.SetSize(4);
    response[0] = 1;
    response[1] = id;
    response[2]["code"] = errorCode;
    response[2]["message"] = errorMsg;
    response[3].SetNull();
}

} // namespace anyrpc
