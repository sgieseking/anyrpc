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

#ifndef ANYRPC_HTTP_H_
#define ANYRPC_HTTP_H_

namespace anyrpc
{
namespace internal
{

//! Process an HTTP header
/* !
 *  The HttpHeader base class is used to process the HTTP header into
 *  the various parts and verify that the required parts are present.
 */
class ANYRPC_API HttpHeader
{
public:
    HttpHeader();
    virtual ~HttpHeader() {}

    //! Initialize the data fields for a new header to be processed.
    virtual void Initialize();

    //! States for the processing
    enum ResultEnum { HEADER_COMPLETE, HEADER_INCOMPLETE, HEADER_FAULT };

    //! Process additional header data and return the state
    ResultEnum ProcessHeaderData(const char* buffer, std::size_t length, bool eof);

    //! Find a character in the string with a defined length
    const char* FindChar(const char* str, std::size_t length, char c);

    std::string& GetHttpVersion()   { return httpVersion_; }
    int GetContentLength()          { return contentLength_; }
    bool GetKeepAlive()             { return keepAlive_; }
    std::size_t GetBodyStartPos()   { return startIndex_; }
    std::string& GetContentType()   { return contentType_; }

protected:
    log_define("AnyRPC.HttpHeader");

    //! Process the first header line into three parts
    virtual ResultEnum ProcessFirstLine(std::string &line);
    //! Process a header line into a key and value pair
    virtual ResultEnum ProcessLine(std::string &line);

    //! Process the first header line specific to a request or response
    virtual ResultEnum ProcessFirstLine(std::string &first, std::string &second, std::string &third) = 0;
    //! Process a header line as a key and value specific to a request or response
    virtual ResultEnum ProcessLine(std::string &key, std::string &value) = 0;
    //! Verify that the header to acceptable
    virtual ResultEnum Verify() = 0;

    std::string httpVersion_;       //!< HTTP version field from the first line
    std::string contentType_;       //!< Info from the content-type field
    int contentLength_;             //!< Info from the content-length field
    bool keepAlive_;                //!< Indication whether the connection should be kept alive after processing

private:
    std::size_t startIndex_;        //!< Offset from the start of the buffer to continue processing
    ResultEnum headerResult_;       //!< Current result from the processing
};

////////////////////////////////////////////////////////////////////////////////

//! Additional HTTP header processing for a request
/*!
 *  Pull additional fields from the header that are specific to a request.
 *  Perform verification of the header specific to a request.
 */
class ANYRPC_API HttpRequest : public HttpHeader
{
public:
    HttpRequest() {}
    virtual void Initialize();

    std::string& GetMethod()        { return method_; }
    std::string& GetRequestUri()    { return requestUri_; }
    std::string& GetHost()          { return host_; }

protected:
    virtual ResultEnum ProcessFirstLine(std::string &first, std::string &second, std::string &third);
    virtual ResultEnum ProcessLine(std::string &key, std::string &value);
    virtual ResultEnum Verify();

private:
    std::string method_;            //!< Request method from the first line
    std::string requestUri_;        //!< Request URI from the first line
    std::string host_;              //!< Info from the host field
};

////////////////////////////////////////////////////////////////////////////////

//! Additional HTTP header processing for a response
/*!
 *  Pull additional fields from the header that are specific to a response.
 *  Perform verification of the header specific to a response.
 */
class ANYRPC_API HttpResponse : public HttpHeader
{
public:
    HttpResponse() {}
    virtual void Initialize();

    std::string& GetResponseCode()  { return responseCode_; }
    std::string& GetResponseString(){ return responseString_; }

protected:
    virtual ResultEnum ProcessFirstLine(std::string &first, std::string &second, std::string &third);
    virtual ResultEnum ProcessLine(std::string &key, std::string &value);
    virtual ResultEnum Verify();

private:
    std::string responseCode_;          //!< Response code from the first line
    std::string responseString_;        //!< Response string from the first line
};

} // namespace internal
} // namespace anyrpc

#endif // ANYRPC_HTTP_H_
