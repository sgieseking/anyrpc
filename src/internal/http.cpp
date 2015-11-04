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
#include "anyrpc/internal/http.h"

namespace anyrpc
{
namespace internal
{
HttpHeader::HttpHeader()
{
    Initialize();
}

void HttpHeader::Initialize()
{
    startIndex_ = 0;
    httpVersion_.clear();
    contentType_.clear();
    contentLength_ = -1;
    keepAlive_ = true;
    headerResult_ = HEADER_INCOMPLETE;
}

HttpHeader::ResultEnum HttpHeader::ProcessHeaderData(const char* buffer, size_t length, bool eof)
{
    log_trace();
    if (length < startIndex_)
    {
        log_warn("Incorrect length=" << length << ", previous start=" << startIndex_);
        headerResult_ = HEADER_FAULT;
    }
    else
        while (headerResult_ == HEADER_INCOMPLETE)
        {
            const char* endLine = FindChar(buffer+startIndex_, length-startIndex_, '\n');
            if (endLine == NULL)
            {
                if (eof)
                {
                    log_warn("EOF before header fully parsed");
                    headerResult_ = HEADER_FAULT;
                }
                break;
            }
            size_t lineLength = endLine - buffer - startIndex_;
            if ((lineLength > 0) && (*(endLine-1) == '\r'))
                lineLength--;

            std::string line(buffer+startIndex_, lineLength);

            if (startIndex_ == 0)
                headerResult_ = ProcessFirstLine(line);
            else
                headerResult_ = ProcessLine(line);

            startIndex_ = endLine - buffer + 1;
        }
    return headerResult_;
}

const char* HttpHeader::FindChar(const char* str, size_t length, char c)
{
    while (length > 0)
    {
        if (*str == c)
            return str;
        str++;
        length--;
    }
    return NULL;
}

HttpHeader::ResultEnum HttpHeader::ProcessFirstLine(std::string &line)
{
    log_trace();
    // Parse the first element of the line
    size_t endPos1 = line.find(' ');
    if (endPos1 == std::string::npos)
    {
        log_warn("Bad first line: " << line);
        return HEADER_FAULT;
    }
    std::string first(line, 0, endPos1);

    // Parse the second element of the line
    size_t endPos2 = line.find(' ', endPos1+1);
    if (endPos2 == std::string::npos)
    {
        log_warn("Bad first line: " << line);
        return HEADER_FAULT;
    }
    std::string second(line, endPos1+1, endPos2-endPos1-1);

    // Parse the third element of the line
    std::string third(line, endPos2+1, std::string::npos);

    log_debug("first=" << first << ", second=" << second << ",third=" << third);
    return ProcessFirstLine(first, second, third);
}

HttpHeader::ResultEnum HttpHeader::ProcessLine(std::string &line)
{
    log_trace();
    if (line.length() == 0)
        return Verify();

    // parse the key
    std::size_t startKey = line.find_first_not_of(" \t");
    if (startKey == std::string::npos)
    {
        log_warn("Invalid key: " << line);
        return HEADER_FAULT;
    }
    size_t endKey = line.find(':', startKey);
    if (endKey == std::string::npos)
    {
        log_warn("Invalid key: start=" << startKey << ", end=" << endKey << ", line=" << line);
        return HEADER_FAULT;
    }
    std::string key(line, startKey, endKey-startKey);

    // make key lower case
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    // parse the value
    size_t startValue = line.find_first_not_of(" \t", endKey+1);
    if (startValue == std::string::npos)
    {
        log_warn("Invalid value: " << line);
        return HEADER_FAULT;
    }
    size_t endValue = line.find_last_not_of(" \t");
    std::string value(line, startValue, endValue-startValue+1);

    log_debug("key=" << key << ", value=" << value);
    return ProcessLine(key, value);
}

////////////////////////////////////////////////////////////////////////////////

void HttpRequest::Initialize()
{
    HttpHeader::Initialize();
    method_.clear();
    requestUri_.clear();
    host_.clear();
}

HttpHeader::ResultEnum HttpRequest::ProcessFirstLine(std::string &first, std::string &second, std::string &third)
{
    log_trace();

    // Set the method
    method_ = first;

    // Set the Uri
    requestUri_ = second;

    // Set the HTTP version
    if (third.compare(0, 5, "HTTP/") != 0)
    {
        log_warn("HTTP version not found: " << third);
        return HEADER_FAULT;
    }
    httpVersion_.assign(third, 5, std::string::npos);

    // set the default for keepAlive
    if (httpVersion_.compare("1.0") == 0)
        keepAlive_ = false;

    return HEADER_INCOMPLETE;
}

HttpHeader::ResultEnum HttpRequest::ProcessLine(std::string &key, std::string &value)
{
    log_trace();
    if (key.compare("content-length") == 0)
    {
        if (contentLength_ != -1)
        {
            log_warn("Content length already specified");
            return HEADER_FAULT;
        }
        contentLength_ = atoi(value.c_str());
        if (contentLength_ < 0)
        {
            log_warn("Invalid content-length specified: " << contentLength_ << ", " << value);
            return HEADER_FAULT;
        }
    }
    else if (key.compare("host") == 0)
    {
        if (host_.length() > 0)
        {
            log_warn("Host already specified: " << host_ << ", new host=" << value);
        }
        host_ = value;
    }
    else if (key.compare("content-type") == 0)
    {
        if (contentType_.length() > 0)
        {
            log_warn("Content-type already specified: " << contentType_ << ", new Content-type=" << value);
            return HEADER_FAULT;
        }
        contentType_ = value;
    }
    else if (key.compare("connection") == 0)
    {
        // make value lower case
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (value.compare("keep-alive") == 0)
            keepAlive_ = true;
        else if (value.compare("close") == 0)
            keepAlive_ = false;
    }

    return HEADER_INCOMPLETE;
}

HttpHeader::ResultEnum HttpRequest::Verify()
{
    log_trace();
    if (httpVersion_.compare("1.1") == 0)
    {
        if (host_.length() == 0)
        {
            log_warn("Host must be specified with HTTP/1.1");
            return HEADER_FAULT;
        }
    }
    if ((contentLength_ < 0) && (method_ == "POST"))
    {
        log_warn("Invalid content length: " << contentLength_);
        return HEADER_FAULT;
    }
    return HEADER_COMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

void HttpResponse::Initialize()
{
    HttpHeader::Initialize();
    responseCode_.clear();
    responseString_.clear();
}

HttpHeader::ResultEnum HttpResponse::ProcessFirstLine(std::string &first, std::string &second, std::string &third)
{
    // Parse the HTTP version
    if (first.compare(0, 5, "HTTP/") != 0)
    {
        log_warn("HTTP version not found: " << first);
        return HEADER_FAULT;
    }
    httpVersion_.assign(first, 5, std::string::npos);

    responseCode_ = second;

    responseString_ = third;

    // set the default for keepAlive
    if (httpVersion_.compare("1.0") == 0)
        keepAlive_ = false;

    return HEADER_INCOMPLETE;
}

HttpHeader::ResultEnum HttpResponse::ProcessLine(std::string &key, std::string &value)
{
    if (key.compare("content-length") == 0)
    {
        if (contentLength_ != -1)
        {
            log_warn("Content length already specified");
            return HEADER_FAULT;
        }
        contentLength_ = atoi(value.c_str());
        if (contentLength_ < 0)
        {
            log_warn("Invalid content-length specified: " << contentLength_ << ", " << value);
            return HEADER_FAULT;
        }
    }
    else if (key.compare("content-type") == 0)
    {
        if (contentType_.length() > 0)
        {
            log_warn("Content-type already specified: " << contentType_ << ", new Content-type=" << value);
            return HEADER_FAULT;
        }
        contentType_ = value;
    }
    else if (key.compare("connection") == 0)
    {
        // make value lower case
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (value.compare("keep-alive") == 0)
            keepAlive_ = true;
        else if (value.compare("close") == 0)
            keepAlive_ = false;
    }

    return HEADER_INCOMPLETE;
}

HttpHeader::ResultEnum HttpResponse::Verify()
{
    if (contentLength_ < 0)
    {
        log_warn("Invalid content length: " << contentLength_);
        return HEADER_FAULT;
    }
    return HEADER_COMPLETE;
}

} // namespace internal
} // namespace anyrpc
