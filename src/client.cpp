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
#include "anyrpc/internal/time.h"

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#endif  // _WIN32

namespace anyrpc
{

#if defined(ANYRPC_THREADING)
std::atomic<unsigned> ClientHandler::nextId_(1);
#else
unsigned ClientHandler::nextId_ = 1;
#endif // defined(ANYRPC_THREADING)

unsigned ClientHandler::GetNextId()
{
    return nextId_++;
}

void ClientHandler::GenerateFaultResult(int errorCode, std::string const& errorMsg, Value& result)
{
    result["code"] = errorCode;
    result["message"] = errorMsg;
}

////////////////////////////////////////////////////////////////////////////////

Client::Client(ClientHandler* handler)
{
    handler_ = handler;
    port_ = 0;
    timeout_ = 60000;
    responseAllocated_ = false;
    responseProcessed_ = false;
    ResetReceiveBuffer();
    ResetTransaction();
}

Client::Client(ClientHandler* handler, const char* host, int port)
{
    handler_ = handler;
    host_ = host;
    port_ = port;
    timeout_ = 60000;
    responseAllocated_ = false;
    responseProcessed_ = false;
    ResetReceiveBuffer();
    ResetTransaction();
}

Client::~Client()
{
    if (responseAllocated_)
        free(response_);
    Close();
}

bool Client::Call(const char* method, Value& params, Value& result)
{
    log_trace();
    gettimeofday( &startTime_, 0 );
    result.SetInvalid();

    PreserveReceiveBuffer();
    ResetTransaction();

    if (Connect() &&
        GenerateRequest(method, params) &&
        GenerateHeader())
    {
        if (!WriteRequest(result))
        {
            // retry the connection
            Close();
            if (!Connect() ||
                !WriteRequest(result))
            {
                Reset();
                return false;
            }
        }
        // continue with the processing
        if (ReadHeader(result) &&
            ReadResponse(result))
        {
            switch (ProcessResponse(result))
            {
                case ProcessResponseSuccess       : return true;
                case ProcessResponseErrorKeepOpen : return false;
                default                           : ; // continue processing
            }
        }
    }
    Reset();
    return false;
}

bool Client::Post(const char* method, Value& params, Value& result)
{
    log_trace();
    gettimeofday( &startTime_, 0 );
    result.SetInvalid();

    PreserveReceiveBuffer();
    ResetTransaction();

    if (Connect() &&
        GenerateRequest(method, params) &&
        GenerateHeader())
    {
        if (!WriteRequest(result))
        {
            // retry the connection
            Close();
            if (!Connect() ||
                !WriteRequest(result))
            {
                Reset();
                return false;
            }
        }
        return true;
    }
    Reset();
    return false;
}

bool Client::GetPostResult(Value& result)
{
    log_trace();
    gettimeofday( &startTime_, 0 );
    result.SetInvalid();

    if (responseProcessed_)
    {
        PreserveReceiveBuffer();
        ResetTransaction();
    }

    if (socket_.IsConnected(0) &&
        ReadHeader(result) &&
        ReadResponse(result))
    {
        switch (ProcessResponse(result))
        {
            case ProcessResponseSuccess       : return true;
            case ProcessResponseErrorKeepOpen : return false;
            default                           : ; // continue processing
        }
    }

    Reset();
    return false;
}

bool Client::Notify(const char* method, Value& params, Value& result)
{
    log_trace();
    gettimeofday( &startTime_, 0 );
    result.SetInvalid();

    PreserveReceiveBuffer();
    ResetTransaction();

    if (Connect() &&
        GenerateRequest(method, params, true) &&
        GenerateHeader())
    {
        if (!WriteRequest(result))
        {
            // retry the connection
            Close();
            if (!Connect() ||
                !WriteRequest(result))
            {
                Reset();
                return false;
            }
        }
        // Not all notification require a response
        if (!TransportHasNotifyResponse())
        {
            requestId_.pop_front();
            return true;
        }
        // continue with the processing response
        if (ReadHeader(result) &&
            ReadResponse(result))
        {
            // don't process the response for a notification
            requestId_.pop_front();
            result.SetNull();
            return true;
        }
    }
    Reset();
    return false;
}

void Client::Reset()
{
    Close();
    ResetReceiveBuffer();
    ResetTransaction();
    requestId_.clear();
}

void Client::ResetTransaction()
{
    contentLength_ = 0;
    if (responseAllocated_)
        free(response_);
    response_ = 0;
    responseAllocated_ = false;
    responseProcessed_ = false;
    contentAvail_ = 0;
    header_.Clear();
    request_.Clear();
}

void Client::PreserveReceiveBuffer()
{
    // only need to consider preserving data if we didn't need to allocate a new buffer
    // if a new buffer was allocated, then only the required data would be read
    if (!responseAllocated_)
    {
        if (response_ != 0)
        {
            // a response was processed so there might be data left in the buffer
            if (contentAvail_ > contentLength_)
            {
                // there is data left in the buffer move it to the start of the buffer
                char* responseEnd = response_ + contentLength_;
                size_t dataRemaining = contentAvail_ - contentLength_;
                log_debug("PreserveReceiveBuffer: Preserve data: " << dataRemaining << " bytes");
                memmove(buffer_, responseEnd, dataRemaining);
                bufferLength_ = dataRemaining;
            }
            else
            {
                log_debug("PreserveReceiveBuffer: No data to preserve");
                bufferLength_ = 0;
            }
        }
        else
            log_debug("PreserveReceiveBuffer: Keep any previous data: " << bufferLength_ << " bytes");
    }
    else
    {
        log_debug("PreserveReceiveBuffer: Reset buffer");
        bufferLength_ = 0;
    }
    responseProcessed_ = true;
}

unsigned Client::GetTimeLeft()
{
    struct timeval currentTime;
    gettimeofday( &currentTime, 0 );

    int timeUsed = MilliTimeDiff(currentTime,startTime_);

    int timeLeft = std::max(0,(int)timeout_ - timeUsed);

    log_debug("GetTimeLeft: timeout=" << timeout_ << ", timeLeft=" << timeLeft << ", timeUsed=" << timeUsed);
    return timeLeft;
}

//! Start the client and connect to server
bool Client::Start()
{
    Value result = Value(ValueType::InvalidType);
    gettimeofday(&startTime_, 0);
    if (!Connect())
    {
        Reset();
        log_warn("Could not connect to server.");
        return false;
    }
    return true;
}

bool Client::Connect()
{
    log_trace();
    if (socket_.IsConnected(0))
    {
        log_debug("Already connected");
        return true;
    }
    // Close the connection but keep any data that has been setup for the next message
    Close();
    ResetReceiveBuffer();

    log_debug("Create a new connection");
    socket_.Create();
    socket_.SetNonBlocking();

    socket_.Connect(host_.c_str(), port_);

    socket_.SetKeepAlive();
    socket_.SetTcpNoDelay();

    if (!socket_.IsConnected(GetTimeLeft()))
    {
        socket_.Close();
        return false;
    }
    return true;
}

bool Client::GenerateRequest(const char* method, Value& params, bool notification)
{
    unsigned requestId;

    bool result = handler_->GenerateRequest(method,params,request_,requestId,notification);
    requestId_.push_back(requestId);
    return result;
}

bool Client::WriteRequest(Value& result)
{
    log_trace();

    size_t bytesWritten;
    size_t bytesToSend;

    // write the header - it make be in several segments
    size_t headerBytesWritten = 0;
    while (headerBytesWritten < header_.Length())
    {
        const char* buffer = header_.GetBuffer(headerBytesWritten, bytesToSend);
        if (!socket_.Send(buffer, bytesToSend, bytesWritten, GetTimeLeft()))
            return false;

        headerBytesWritten += bytesWritten;

        if (bytesToSend < bytesWritten)
            return false;   // timeout
    }

    // write the request - it make be in several segments
    size_t requestBytesWritten = 0;
    while (requestBytesWritten < request_.Length())
    {
        const char* buffer = request_.GetBuffer(requestBytesWritten, bytesToSend);
        if (!socket_.Send(buffer, bytesToSend, bytesWritten, GetTimeLeft()))
            return false;

        requestBytesWritten += bytesWritten;

        if (bytesToSend < bytesWritten)
            return false;   // timeout
    }

    return true;
}

bool Client::ReadHeader(Value& result)
{
    log_trace();
    // Read available data
    socket_.SetTimeout(0);
    while (true)
    {
        size_t bytesRead;
        bool eof;
        socket_.Receive(buffer_+bufferLength_, MaxBufferLength-bufferLength_, bytesRead, eof, 0);
        if (socket_.FatalError())
        {
            log_warn("error while reading header: " << socket_.GetLastError() << ", bytesRead=" << bytesRead);
            return false;
        }
        bufferLength_ += bytesRead;
        log_info("read=" << bytesRead << ", total=" << bufferLength_);

        switch (ProcessHeader(eof))
        {
            case HEADER_COMPLETE : return true;
            case HEADER_FAULT : return false;
        }
        unsigned timeLeft = GetTimeLeft();
        if (timeLeft == 0)
        {
            handler_->GenerateFaultResult(AnyRpcErrorTransportError,"Timeout reading response header",result);
            break;
        }
        socket_.WaitReadable(timeLeft);
    }
    return false;
}

bool Client::ReadResponse(Value& result)
{
    log_trace();
    if (contentAvail_ >= contentLength_)
        return true;

    size_t bytesRead;
    bool eof;
    bool receiveResult = socket_.Receive(response_+contentAvail_, contentLength_-contentAvail_, bytesRead, eof, GetTimeLeft());
    contentAvail_ += bytesRead;
    response_[contentAvail_] = 0;
    if (!receiveResult)
        handler_->GenerateFaultResult(AnyRpcErrorTransportError,"Failed reading response",result);
    return receiveResult;
}

ProcessResponseEnum Client::ProcessResponse(Value& result, bool notification)
{
    log_trace();
    unsigned requestId = 0;
    // There should be an id in the list, but in case there isn't handle with default id of 0
    if (!requestId_.empty())
    {
        requestId = requestId_.front();
        requestId_.pop_front();
    }
    responseProcessed_ = true;
    return handler_->ProcessResponse(response_,contentLength_,result,requestId,notification);
}

////////////////////////////////////////////////////////////////////////////////

bool HttpClient::GenerateHeader()
{
    log_trace();
    header_ << "POST /RPC2 HTTP/1.1\r\n";
    header_ << "User-Agent: " << ANYRPC_APP_NAME << " v" << ANYRPC_VERSION_STRING << "\r\n";
    header_ << "Host: " << host_ << ":" << port_ << "\r\n";
    header_ << "Content-Type: " << contentType_ << "\r\n";
    header_ << "Accept: " << contentType_ << "\r\n";
    header_ << "Content-length: " << request_.Length() << "\r\n";
    header_ << "\r\n";

    return true;
}

int HttpClient::ProcessHeader(bool eof)
{
    log_trace();
    switch (httpResponseState_.ProcessHeaderData(buffer_, bufferLength_, eof))
    {
        case internal::HttpHeader::HEADER_FAULT         : return HEADER_FAULT;
        case internal::HttpHeader::HEADER_INCOMPLETE    : return HEADER_INCOMPLETE;
        default                                         : ; // continue processing
    }

    size_t bodyStartPos = httpResponseState_.GetBodyStartPos();
    size_t bufferSpaceAvail = MaxBufferLength - bodyStartPos;

    contentLength_ = std::max(0,httpResponseState_.GetContentLength());
    contentAvail_ = bufferLength_ - bodyStartPos;

    if (contentLength_ > MaxContentLength)
    {
        log_warn("Content-length too large=" << contentLength_ << ", max allowed=" << MaxContentLength);
        return HEADER_FAULT;
    }
    if (contentLength_ > bufferSpaceAvail)
    {
        // try to malloc the buffer space needed
        response_ = static_cast<char*>(malloc(contentLength_+1));
        if (response_ == 0)
        {
            log_warn("Could not allocate space=" << contentLength_);
            return HEADER_FAULT;
        }
        // copy the content that was already read to the new space
        memcpy(response_,buffer_+bodyStartPos,contentAvail_);
        responseAllocated_ = true;
    }
    else
    {
        responseAllocated_ = false;
        response_ = buffer_ + bodyStartPos;
    }
    response_[contentAvail_] = 0;

    log_info("specified content length is " << contentLength_);

    if (httpResponseState_.GetResponseCode() != "200")
    {
        log_warn("Response code indicates problem, code = " << httpResponseState_.GetResponseCode() << ", string = " << httpResponseState_.GetResponseString());
        return HEADER_FAULT;
    }

    return HEADER_COMPLETE;
}

ProcessResponseEnum HttpClient::ProcessResponse(Value& result, bool notification)
{
    log_trace();
    unsigned requestId = 0;
    // There should be an id in the list, but in case there isn't handle with default id of 0
    if (!requestId_.empty())
    {
        requestId = requestId_.front();
        requestId_.pop_front();
    }
    responseProcessed_ = true;
    ProcessResponseEnum processResult = handler_->ProcessResponse(response_,contentLength_,result,requestId,notification);
    if (!httpResponseState_.GetKeepAlive())
    {
        log_info("Http response header indicates to close connection");
        Close();
    }
    return processResult;
}

////////////////////////////////////////////////////////////////////////////////

bool TcpClient::GenerateHeader()
{
    log_trace();
    header_ << request_.Length() << ":";
    log_debug("Request length=" << request_.Length());

    // add the comma separator to the request buffer
    request_ << ',';

    return true;
}

int TcpClient::ProcessHeader(bool eof)
{
    log_trace();
    // search for the length/body separator, ':'
    char* body = 0;
    char* header = buffer_;
    char* end = buffer_ + bufferLength_;
    for (char* current = buffer_; current < end; current++)
    {
        if (*current == ':')
        {
            body = current + 1;
            break;
        }
    }

    // If we haven't gotten the entire header yet, return (keep reading)
    if (body == 0)
    {
        // EOF in the middle of a request is an error, otherwise its ok
        if (eof)
        {
            log_warn("EOF while reading header");
            return HEADER_FAULT;
        }
        return HEADER_INCOMPLETE;  // Keep reading
    }

    // check for comma separator
    if (commaExpected_)
    {
        if (*header != ',')
        {
            log_warn("Expected comma to separate messages");
            log_warn("bufferLength=" << bufferLength_ << ", buffer=" << buffer_);
            return HEADER_FAULT;
        }
        header++;
    }

    int contentLength = atoi(header);
    if (contentLength <= 0)
    {
        log_warn("Invalid string length specified " << contentLength_);
        log_warn("bufferLength=" << bufferLength_ << ", buffer=" << buffer_);
        return HEADER_FAULT;
    }
    contentLength_ = static_cast<size_t>(contentLength);

    size_t bufferSpaceAvail = buffer_ + MaxBufferLength - body;
    contentAvail_ = end - body;

    if (contentLength_ > MaxContentLength)
    {
        log_warn("String length too large=" << contentLength_ << ", max allowed=" << MaxContentLength);
        return HEADER_FAULT;
    }
    if (contentLength_ > bufferSpaceAvail)
    {
        // try to malloc the buffer space needed
        response_ = static_cast<char*>(malloc(contentLength_+1));
        if (response_ == 0)
        {
            log_warn("Could not allocate space=" << contentLength_);
            return HEADER_FAULT;
        }
        // copy the content that was already read to the new space
        memcpy(response_,body,contentAvail_);
        responseAllocated_ = true;
    }
    else
    {
        responseAllocated_ = false;
        response_ = body;
    }
    response_[contentAvail_] = 0;
    log_info("specified content length is " << contentLength_);

    // a comma is expected to separate the next message
    commaExpected_ = true;

    return HEADER_COMPLETE;
}

} // namespace anyrpc
