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
#include "anyrpc/xml/xmlwriter.h"
#include "anyrpc/xml/xmlreader.h"
#include "anyrpc/xml/xmlclient.h"

namespace anyrpc
{

//! Only need one instance of the XmlClientHandler since there is no local storage.
static XmlClientHandler XmlClientHandler;

XmlHttpClient::XmlHttpClient() : HttpClient(&XmlClientHandler, "text/xml") {}

XmlHttpClient::XmlHttpClient(const char* host, int port) :
        HttpClient(&XmlClientHandler, "text/xml", host, port) {}

////////////////////////////////////////////////////////////////////////////////

XmlTcpClient::XmlTcpClient() : TcpClient(&XmlClientHandler) {}

XmlTcpClient::XmlTcpClient(const char* host, int port) :
        TcpClient(&XmlClientHandler, host, port) {}

////////////////////////////////////////////////////////////////////////////////

bool XmlClientHandler::GenerateRequest(const char* method, Value& params, Stream& os, unsigned& requestId, bool notification)
{
    log_trace();

    os.Put("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n");
    os.Put("<methodCall><methodName>");
    os.Put(method);
    os.Put("</methodName><params>");

    XmlWriter xmlStrWriter(os);
    if (params.IsArray())
    {
        // write each element of the array as a separate param
        for (int i=0; i<(int)params.Size(); i++)
        {
            os.Put("<param>");
            params[i].Traverse(xmlStrWriter);
            os.Put("</param>");
        }
    }
    else
    {
        // put all of the data in a single param
        os.Put("<param>");
        params.Traverse(xmlStrWriter);
        os.Put("</param>");
    }

    os.Put("</params></methodCall>");

    return true;
}

ProcessResponseEnum XmlClientHandler::ProcessResponse(char* response, int length, Value& result, unsigned requestId, bool notification)
{
    log_trace();
    ProcessResponseEnum processResponse = ProcessResponseErrorClose;
    try
    {
        Document doc;

        InSituStringStream sstream(response, length);
        XmlReader reader(sstream);

        reader.ParseResponse(doc);
        if (reader.HasParseError())
        {
            std::stringstream message;
            message << "Response parse error, offset=" << reader.GetErrorOffset();
            message << ", code=" << reader.GetParseErrorCode();
            message << ", message=" << reader.GetParseErrorStr();
            GenerateFaultResult(AnyRpcErrorResponseParseError, message.str(), result);
        }
        else if (doc.GetValue().IsMap())
        {
            // looks like a fault response - verify that it is properly formated
            result.Assign( doc.GetValue() );
            if (result.HasMember("faultCode") && result.HasMember("faultString"))
            {
                // standard xmlrpc fault codes - copy fault codes to the standard names
                GenerateFaultResult(result["faultCode"].GetInt(), result["faultString"].GetString(), result);
                // keep the connection open
                processResponse =  ProcessResponseErrorKeepOpen;
            }
            else
                GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, wrong fault fields", result);
        }
        else if (!doc.GetValue().IsArray() || (doc.GetValue().Size() != 1))
            GenerateFaultResult(AnyRpcErrorInvalidResponse, "Invalid response, wrong field types", result);
        else
        {
            result.Assign( doc.GetValue()[0] );
            processResponse = ProcessResponseSuccess;
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
