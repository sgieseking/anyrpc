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
#include "anyrpc/json/jsonwriter.h"
#include "anyrpc/json/jsonreader.h"
#include "anyrpc/json/jsonserver.h"

namespace anyrpc
{

static void JsonExecuteSingleRequest(MethodManager* manager, Value& message, Value& response);
static void JsonGenerateResponse(Value& result, Value& id, Value& response);
static void JsonGenerateFaultResponse(int errorCode, std::string const& errorMsg, Value& id, Value& response);

log_define("AnyRPC.RpcHandler");

////////////////////////////////////////////////////////////////////////////////

bool JsonRpcHandler(MethodManager* manager, char* request, size_t length, Stream &response)
{
    Document doc;
    Value valueResponse;
    Value nullValue;
    nullValue.SetNull();

    InSituStringStream sstream(request, length);
    JsonReader reader(sstream);
    reader.ParseStream(doc);
    if (reader.HasParseError())
        JsonGenerateFaultResponse(AnyRpcErrorParseError, "Parse error", nullValue, valueResponse);
    else
    {
        Value message;
        message.Assign( doc.GetValue() );

        if (message.IsMap())
        {
            JsonExecuteSingleRequest(manager,message,valueResponse);
        }
        else if (message.IsArray() && (message.Size() > 0))
        {
            // multi-call request
            int outIndex = 0;
            for (int i=0; i<(int)message.Size(); i++)
            {
                Value singleResponse;
                JsonExecuteSingleRequest(manager,message[i],singleResponse);
                if (singleResponse.IsValid())
                    valueResponse[outIndex++].Assign(singleResponse);
            }
        }
        else
        {
            log_debug("message type = " << message.GetType() << ", Size = " << message.Size());
            JsonGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", nullValue, valueResponse);
        }
    }

    if (valueResponse.IsInvalid())
        // notification doesn't produce a response message
        return false;

    JsonWriter jsonStrWriter(response);
    valueResponse.Traverse(jsonStrWriter);

    return true;
}

static void JsonExecuteSingleRequest(MethodManager* manager, Value& message, Value& response)
{
    Value& method = message["method"];
    Value& id = message["id"];
    Value& rpc = message["jsonrpc"];
    Value& params = message["params"];

    if (method.IsInvalid() || !method.IsString())
        JsonGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", id, response);
    else if (!rpc.IsString() || (strcmp(rpc.GetString(), "2.0") != 0))
        JsonGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", id, response);
    else if (params.IsInvalid())
        JsonGenerateFaultResponse(AnyRpcErrorInvalidRequest, "Invalid Request", id, response);
    else
    {
        Value result;
        result.SetNull();

        std::string methodName = method.GetString();
        try
        {
            if (!manager->ExecuteMethod(methodName, params, result))
                JsonGenerateFaultResponse(AnyRpcErrorMethodNotFound, "Method not found", id, response);
            else if (id.IsValid())
                JsonGenerateResponse(result, id, response);
        }
        catch (const AnyRpcException& fault)
        {
            JsonGenerateFaultResponse(fault.GetCode(), fault.GetMessage(), id, response);
        }
    }
}

static void JsonGenerateResponse(Value& result, Value& id, Value& response)
{
    if (id.IsValid())
    {
        response["jsonrpc"] = "2.0";
        response["id"] = id;
        response["result"].Assign(result);
    }
    else
        response.SetInvalid();
}

static void JsonGenerateFaultResponse(int errorCode, std::string const& errorMsg, Value& id, Value& response)
{
    response["jsonrpc"] = "2.0";
    if (id.IsValid())
        response["id"] = id;
    response["error"]["code"] = errorCode;
    response["error"]["message"] = errorMsg;
}

} // namespace anyrpc
