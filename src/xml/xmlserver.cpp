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
#include "anyrpc/xml/xmlwriter.h"
#include "anyrpc/xml/xmlreader.h"
#include "anyrpc/xml/xmlserver.h"

namespace anyrpc
{

static void XmlExecuteMultiCall(MethodManager* manager, Value &params, Value &result);
static void XmlGenerateResponse(Value &result, Stream &response);
static void XmlGenerateFaultResponse(int errorCode, std::string const& errorMsg, Stream &response);
static void XmlGenerateFaultValue(int errorCode, std::string const& errorMsg, Value &faultValue);

log_define("AnyRPC.RpcHandler");

////////////////////////////////////////////////////////////////////////////////

bool XmlRpcHandler(MethodManager* manager, char* request, size_t length, Stream &response)
{
    InSituStringStream sstream(request, length);
    XmlReader reader(sstream);
    Document doc;

    std::string methodName = reader.ParseRequest(doc);
    log_info("MethodName=" << methodName);

    if (reader.HasParseError())
        XmlGenerateFaultResponse(AnyRpcErrorParseError, "Parse error", response);
    else
    {
        Value params;
        Value result;

        params.Assign(doc.GetValue());
        log_info("Execute method");
        if (methodName == "system.multicall")
        {
            if (!params.IsArray() || (params.Size() != 1) || !params[0].IsArray())
                XmlGenerateFaultResponse(AnyRpcErrorInvalidParams, "Invalid method parameters", response);
            else
            {
                XmlExecuteMultiCall(manager,params[0],result);
                if (result.IsInvalid())
                    result.SetString("");
                XmlGenerateResponse(result, response);
           }
        }
        else
        {
            try
            {
                if (manager->ExecuteMethod(methodName,params,result))
                {
                    if (result.IsInvalid())
                        result = "";
                    XmlGenerateResponse(result, response);
                }
                else
                    XmlGenerateFaultResponse(AnyRpcErrorMethodNotFound, "Method not found", response);
            }
            catch (const AnyRpcException& fault)
            {
                XmlGenerateFaultResponse(fault.GetCode(), fault.GetMessage(), response);
            }
        }
    }
    return true;
}

static void XmlExecuteMultiCall(MethodManager* manager, Value &params, Value &result)
{
    log_debug("ExecuteMultiCall: params= " << params);

    result.SetSize(params.Size());
    for (int i=0; i<(int)params.Size(); i++)
    {
        Value singleParams;
        singleParams.Assign(params[i]);
        log_debug("Execute index " << i << ": params= " << singleParams);
        if (!singleParams.IsMap() ||
            !singleParams.HasMember("methodName") ||
            !singleParams["methodName"].IsString() ||
            !singleParams.HasMember("params"))
            XmlGenerateFaultValue(AnyRpcErrorInvalidRequest, "Invalid request", result[i]);
        else
        {
            Value singleResult;
            Value methodName = singleParams["methodName"];
            try
            {
                if (manager->ExecuteMethod(methodName.GetString(),singleParams["params"],singleResult))
                {
                    if (singleResult.IsInvalid())
                        singleResult = "";
                    result[i][0] = singleResult;
                }
                else
                    XmlGenerateFaultValue(AnyRpcErrorMethodNotFound, "Method not found", result[i] );
            }
            catch (const AnyRpcException& fault)
            {
                XmlGenerateFaultValue(fault.GetCode(), fault.GetMessage(), result[i]);
            }
        }
    }
    log_debug("ExecuteMultiCall: result= " << result);
}

static void XmlGenerateResponse(Value &result, Stream &response)
{
    response.Put("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n<methodResponse><params><param>");
    XmlWriter xmlStrWriter(response);
    result.Traverse(xmlStrWriter);
    response.Put("</param></params></methodResponse>\r\n");
}

static void XmlGenerateFaultResponse(int errorCode, std::string const& errorMsg, Stream &response)
{
    Value faultValue;
    XmlGenerateFaultValue(errorCode, errorMsg, faultValue);

    response.Put("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n<methodResponse><fault>");
    XmlWriter xmlStrWriter(response);
    faultValue.Traverse(xmlStrWriter);
    response.Put("</fault></methodResponse>\r\n");
}

static void XmlGenerateFaultValue(int errorCode, std::string const& errorMsg, Value &faultValue)
{
    faultValue["faultCode"] = errorCode;
    faultValue["faultString"] = errorMsg;
}

} // namespace anyrpc
