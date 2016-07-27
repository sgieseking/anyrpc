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
#include "anyrpc/internal/time.h"

namespace anyrpc
{

bool RpcContentHandler::CanProcessContentType(std::string contentType)
{
    if (handler_ == 0)
        return false;

    if (matchAnyContentType_)
        return true;

#if defined(ANYRPC_REGEX)
    return std::regex_match(contentType, requestContentType_);
#else
    return (contentType.find(requestContentType_) != std::string::npos);
#endif // defined(ANYRPC_REGEX)
}

////////////////////////////////////////////////////////////////////////////////

Connection::Connection(SOCKET fd, MethodManager* manager) :
    manager_(manager)
{
    connectionState_ = READ_HEADER;
    lastTransactionTime_ = time(NULL);
    active_ = true;
    bufferLength_ = 0;
    contentLength_ = 0;
    request_ = 0;
    requestAllocated_ = false;
    keepAlive_ = false;
    contentAvail_ = 0;
    headerBytesWritten_ = 0;
    resultBytesWritten_ = 0;
#if defined(ANYRPC_THREADING)
    threadRunning_ = false;
#endif

    socket_.SetFileDescriptor(fd);

    int result;
    result = socket_.SetNonBlocking();
    if (result != 0)
        log_warn("Could not set socket to non-blocking input mode: " << result);

    result = socket_.SetTcpNoDelay();
    if (result != 0)
        log_warn("Could not set socket to no delay: " << result);
}

Connection::~Connection()
{
    log_debug("Connection destructor, fd=" << socket_.GetFileDescriptor());
    if (requestAllocated_)
        free(request_);
}

void Connection::Initialize(bool preserveBufferData)
{
    // only need to consider preserving data if we didn't need to allocate a new buffer
    // if a new buffer was allocated, then only the required data would be read
    if (preserveBufferData && !requestAllocated_)
    {
        if (request_ != 0)
        {
            // a request was processed so there might be data left in the buffer
            if (contentAvail_ > contentLength_)
            {
                // there is data left in the buffer move it to the start of the buffer
                char* requestEnd = request_ + contentLength_;
                size_t dataRemaining = contentAvail_ - contentLength_;
                log_info("Initialize: Preserve data: " << dataRemaining << " bytes");
                memmove(buffer_, requestEnd, dataRemaining);
                bufferLength_ = dataRemaining;
            }
            else
            {
                log_info("Initialize: No data to preserve");
                bufferLength_ = 0;
            }
        }
        else
            log_info("Initialize: Keep previous data: " << bufferLength_ << " bytes");
    }
    else
    {
        log_info("Initialize: Reset buffer");
        bufferLength_ = 0;
    }

    contentLength_ = 0;
    if (requestAllocated_)
    {
        free(request_);
        requestAllocated_ = false;
    }
    request_ = 0;
    header_.Clear();
    response_.Clear();
    contentAvail_ = 0;
    headerBytesWritten_ = 0;
    resultBytesWritten_ = 0;
}

#if defined(ANYRPC_THREADING)
void Connection::StartThread()
{
    threadRunning_ = true;
    thread_ = std::thread(&Connection::ThreadStarter,this);
}

void Connection::StopThread(bool waitForJoin)
{
    log_trace();
    threadRunning_ = false;
    if (waitForJoin && thread_.joinable())
        thread_.join();
}

void Connection::ThreadStarter()
{
    log_trace();
    while (threadRunning_ && !CheckClose())
    {
        Work(100);
    }
    log_debug("Connection thread exiting, fd=" << socket_.GetFileDescriptor());
    // force the close to close since the thread may not be deleted right away
    socket_.Close();
    threadRunning_ = false;
}

void Connection::Work(int ms)
{
    int timeLeft;
    struct timeval startTime;
    struct timeval currentTime;
    gettimeofday( &startTime, 0 );

    do
    {
        //log_debug("Work: timeLeft=" << timeLeft.count());
        // Construct the sets of descriptors we are interested in
        fd_set inFd, outFd;
        FD_ZERO(&inFd);
        FD_ZERO(&outFd);

        // add connection socket
        SOCKET maxFd = socket_.GetFileDescriptor();
        if (WaitForReadability())
            FD_SET(maxFd, &inFd);
        if (WaitForWritability())
            FD_SET(maxFd, &outFd);

        // Check for events
        int nEvents;
        if (ms < 0)
            nEvents = select(static_cast<int>(maxFd) + 1, &inFd, &outFd, NULL, NULL);
        else
        {
            gettimeofday( &currentTime, 0 );
            timeLeft = ms - MilliTimeDiff(currentTime,startTime);

            struct timeval tv;
            tv.tv_sec = timeLeft / 1000;
            tv.tv_usec = (timeLeft - tv.tv_sec*1000) * 1000;
            nEvents = select(static_cast<int>(maxFd) + 1, &inFd, &outFd, NULL, &tv);
        }

        if (nEvents < 0)
            break;

        // Process server events
        SOCKET fd = GetFileDescriptor();
        if ((FD_ISSET(fd, &inFd )) || (FD_ISSET(fd, &outFd)))
            Process();

        gettimeofday( &currentTime, 0 );
        timeLeft = ms - MilliTimeDiff(currentTime,startTime);

    } while (!CheckClose() && ((ms < 0) || (timeLeft > 0)));
}
#endif // defined(ANYRPC_THREADING)

void Connection::Process(bool executeAfterRead)
{
    log_info("Process: fd=" << socket_.GetFileDescriptor());
    bool newMessage = true;
    while (newMessage)
    {
        newMessage = false;
        if ((connectionState_ == READ_HEADER) && (!ReadHeader()))
        {
            connectionState_ = CLOSE_CONNECTION;
            break;
        }
        if ((connectionState_ == READ_REQUEST) && (!ReadRequest()))
        {
            connectionState_ = CLOSE_CONNECTION;
            break;
        }
        // When operating a thread pool, the main thread will read the request
        // but stop to transfer it to a worker thread for execution
        if (executeAfterRead && (connectionState_ == EXECUTE_REQUEST))
        {
            if (!ExecuteRequest())
            {
                connectionState_ = CLOSE_CONNECTION;
                break;
            }
            // for a notification over tcp, there is no data sent back so check for more data to process
            newMessage = (connectionState_ == READ_HEADER) && (bufferLength_ > 0);
        }
        // When operating a thread pool, the main thread might need to continue
        // to write a long response to the socket after the connection is returned
        if (connectionState_ == WRITE_RESPONSE)
        {
            if (!WriteResponse())
            {
                connectionState_ = CLOSE_CONNECTION;
                break;
            }
            newMessage = (connectionState_ == READ_HEADER) && (bufferLength_ > 0);
        }
    }
}

bool Connection::ReadRequest()
{
    // If we don't have the entire request yet, read available data
    if (contentAvail_ < contentLength_)
    {
        size_t bytesRead;
        bool eof;
        if (!socket_.Receive(request_+contentAvail_, contentLength_-contentAvail_, bytesRead, eof))
        {
            log_warn("read error " << socket_.GetLastError());
            Initialize();
            return false;
        }

        contentAvail_ += bytesRead;

        // If we haven't gotten the entire request yet, return (keep reading)
        if (contentAvail_ < contentLength_)
        {
            // EOF in the middle of a request is an error, otherwise its ok
            if (eof)
            {
                log_warn("EOF while reading body");
                Initialize();
                return false;
            }
            return true;  // Keep reading
        }
    }

    // null terminate the request in case it is written to a log file
    request_[contentAvail_] = 0;

    // Otherwise, parse and dispatch the request
    log_info("read " << contentAvail_);

    connectionState_ = EXECUTE_REQUEST;

    return true;    // Continue monitoring this source
}

bool Connection::WriteResponse()
{
    // Try to write the response
    log_info("WriteResponse");
    lastTransactionTime_ = time(NULL);

    size_t bytesWritten;
    size_t bytesToSend;

    while (headerBytesWritten_ < header_.Length())
    {
        const char* buffer = header_.GetBuffer(resultBytesWritten_,bytesToSend);
        if (!socket_.Send(buffer, bytesToSend, bytesWritten))
        {
            log_fatal("header write error " << socket_.GetLastError());
            Initialize();
            return false;
        }
        headerBytesWritten_ += bytesWritten;

        if (bytesToSend < bytesWritten)
            // not all of the data was written, need to wait until writable again
            return true;
    }

    // write more of the result/body
    while (resultBytesWritten_ < response_.Length())
    {
        const char* buffer = response_.GetBuffer(resultBytesWritten_,bytesToSend);
        if (!socket_.Send(buffer, bytesToSend, bytesWritten))
        {
            log_fatal("result write error " << socket_.GetLastError());
            Initialize();
            return false;
        }
        resultBytesWritten_ += bytesWritten;

        if (bytesToSend < bytesWritten)
            // not all of the data was written, need to wait until writable again
            return true;
    }
    connectionState_ = READ_HEADER;
    Initialize(keepAlive_);

    return keepAlive_;    // Continue monitoring this source if true
}

////////////////////////////////////////////////////////////////////////////////

void HttpConnection::Initialize(bool preserveBufferData)
{
    Connection::Initialize(preserveBufferData);
    httpRequestState_.Initialize();
}

bool HttpConnection::ReadHeader()
{
    // Read available data
    size_t bytesRead;
    bool eof;
    if (!socket_.Receive(buffer_+bufferLength_, MaxBufferLength-bufferLength_, bytesRead, eof))
    {
        if (eof)
            log_info("Client disconnect: error=" << socket_.GetLastError());
        else
            log_warn("Error while reading header: error=" << socket_.GetLastError() << ", bytesRead=" << bytesRead);
        Initialize();
        return false;
    }
    bufferLength_ += bytesRead;

    log_info("read=" << bytesRead << ", total=" << bufferLength_);
    switch (httpRequestState_.ProcessHeaderData(buffer_, bufferLength_, eof))
    {
        case internal::HttpHeader::HEADER_FAULT         : Initialize(); return false;
        case internal::HttpHeader::HEADER_INCOMPLETE    : return true;
        default                                         : ; // continue processing
    }
    size_t bodyStartPos = httpRequestState_.GetBodyStartPos();
    size_t bufferSpaceAvail = MaxBufferLength - bodyStartPos;

    contentLength_ = httpRequestState_.GetContentLength();
    contentAvail_ = bufferLength_ - bodyStartPos;
    keepAlive_ = httpRequestState_.GetKeepAlive();

    if (contentLength_ > MaxContentLength)
    {
        log_warn("Content-length too large=" << contentLength_ << ", max allowed=" << MaxContentLength);
        Initialize();
        return false;
    }
    if (contentLength_ > bufferSpaceAvail)
    {
        // try to malloc the buffer space needed
        request_ = static_cast<char*>(calloc(contentLength_+1,1));
        if (request_ == 0)
        {
            log_warn("Could not allocate space=" << contentLength_);
            Initialize();
            return false;
        }
        // copy the content that was already read to the new space
        memcpy(request_,buffer_+bodyStartPos,contentAvail_);
        requestAllocated_ = true;
    }
    else
    {
        requestAllocated_ = false;
        request_ = buffer_ + bodyStartPos;
    }
    log_info("specified content length is " << contentLength_);
    log_info("KeepAlive: " << keepAlive_);

    connectionState_ = READ_REQUEST;
    return true;    // Continue monitoring this source
}

bool HttpConnection::ExecuteRequest()
{
    log_debug("ContentLength=" << contentLength_ << ", request=" << request_);

    std::string& requestContentType = httpRequestState_.GetContentType();

    if (httpRequestState_.GetMethod() == "POST")
    {
        // find a handler that will work
        RpcHandlerList::iterator it = handlers_.begin();
        for (;it!= handlers_.end(); it++)
            if ((*it).CanProcessContentType(requestContentType))
                break;
        if (it == handlers_.end())
        {
            log_warn("Content type not supported by server, " << requestContentType);
            Initialize();
            return false;
        }
        else
        {
            (*it).HandleRequest(manager_, request_, contentLength_, response_);

            log_debug("Response length=" << response_.Length());

            std::string& responseContentType = (*it).GetResponseContentType();
            if (responseContentType.length() == 0)
                responseContentType = requestContentType;
            GeneratePOSTResponseHeader(response_.Length(), responseContentType);
        }
    }
    else if (httpRequestState_.GetMethod() == "OPTIONS")
    {
        GenerateOPTIONSResponseHeader();
    }
    else
    {
        GenerateErrorResponseHeader(501, "Not Implemented");
    }

    connectionState_ = WRITE_RESPONSE;
    return true;
}

void HttpConnection::GeneratePOSTResponseHeader(std::size_t bodySize, std::string& contentType)
{
    header_ << "HTTP/1.1 200 OK\r\n";
    header_ << "Server: " << ANYRPC_APP_NAME << " v" << ANYRPC_VERSION_STRING << "\r\n";
    if (keepAlive_)
        header_ << "Connection: keep-alive\r\n";
    else
        header_ << "Connection: close\r\n";
    header_ << "Content-Type: " << contentType << "\r\n";
    header_ << "Content-length: " << static_cast<uint64_t>(bodySize) << "\r\n";
    header_ << "\r\n";
}

void HttpConnection::GenerateOPTIONSResponseHeader()
{
    header_ << "HTTP/1.1 200 OK\r\n";
    header_ << "Access-Control-Allow-Origin: *\r\n";
    header_ << "Access-Control-Allow-Methods: POST\r\n";
    header_ << "Access-Control-Max-Age: 1728000\r\n"; // 20 days max age
    header_ << "Access-Control-Allow-Headers: Content-Type\r\n";
    header_ << "Vary: Accept-Encoding, Origin\r\n";
    header_ << "Keep-Alive: timeout=2, max=100\r\n";
    header_ << "Connection: keep-alive\r\n";
    header_ << "\r\n";
}

void HttpConnection::GenerateErrorResponseHeader(int code, std::string message)
{
    header_ << "HTTP/1.1 " << code << " " << message << "\r\n";
    header_ << "Connection: close\r\n";
    header_ << "\r\n";
}

////////////////////////////////////////////////////////////////////////////////

bool TcpConnection::ReadHeader()
{
    // Read available data
    size_t bytesRead;
    bool eof;
    if (!socket_.Receive(buffer_+bufferLength_, MaxBufferLength-bufferLength_, bytesRead, eof))
    {
        if (eof)
            log_info("Client disconnect: error=" << socket_.GetLastError());
        else
            log_warn("Error while reading header: error=" << socket_.GetLastError() << ", bytesRead=" << bytesRead);
        Initialize();
        return false;
    }
    bufferLength_ += bytesRead;

    log_info("read=" << bytesRead << ", total=" << bufferLength_);

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
            log_warn("Client disconnect: error=" << socket_.GetLastError());
            Initialize();
            return false;
        }
        return true;  // Keep reading
    }

    // check for comma separator
    if (commaExpected_)
    {
        if (*header != ',')
        {
            log_warn("Expected comma to separate messages");
            Initialize();
            return false;
        }
        header++;
    }

    int contentLength = atoi(header);
    if (contentLength <= 0)
    {
        log_warn("Invalid string length specified " << contentLength_);
        Initialize();
        return false;
    }
    contentLength_ = static_cast<size_t>(contentLength);

    size_t bufferSpaceAvail = buffer_ + MaxBufferLength - body;
    contentAvail_ = end - body;

    if (contentLength_ > MaxContentLength)
    {
        log_warn("String length too large=" << contentLength_ << ", max allowed=" << MaxContentLength);
        Initialize();
        return false;
    }
    if (contentLength_ > bufferSpaceAvail)
    {
        // try to malloc the buffer space needed
        request_ = static_cast<char*>(malloc(contentLength_+1));
        if (request_ == 0)
        {
            log_warn("Could not allocate space=" << contentLength_);
            Initialize();
            return false;
        }
        // copy the content that was already read to the new space
        memcpy(request_,body,contentAvail_);
        requestAllocated_ = true;
    }
    else
    {
        requestAllocated_ = false;
        request_ = body;
    }
    request_[contentAvail_] = 0;
    log_info("specified content length is " << contentLength_);

    // a comma is expected to separate the next message
    commaExpected_ = true;

    // always keep alive after message
    keepAlive_ = true;

    connectionState_ = READ_REQUEST;
    return true;    // Continue monitoring this source
}

bool TcpConnection::ExecuteRequest()
{
    log_debug("ContentLength=" << contentLength_ << ", request=" << request_);
    bool sendResponse = handler_(manager_, request_, contentLength_, response_);

    if (sendResponse)
    {
        // generate the header and add comma separator for netstrings format
        log_debug("Response length=" << response_.Length());
        header_ << static_cast<uint64_t>(response_.Length()) << ":";
        response_.Put(',');

        connectionState_ = WRITE_RESPONSE;
    }
    else
    {
        connectionState_ = READ_HEADER;
        Initialize(true);
    }
    return true;
}

} // namespace anyrpc
