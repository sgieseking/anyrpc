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

#ifndef ANYRPC_CONNECTION_H_
#define ANYRPC_CONNECTION_H_

#if defined(ANYRPC_THREADING)
# if defined(__MINGW32__)
#  include "internal/mingw.thread.h"
# else
#  include <thread>
# endif // defined(__MINGW32__)
#endif // defined(ANYRPC_THREADING)

#if defined(ANYRPC_REGEX)
# include <regex>        // this requires a compiler with c++11 support
#endif // defined(ANYRPC_REGEX)

#include "internal/http.h"

namespace anyrpc
{

////////////////////////////////////////////////////////////////////////////////

//! Process an RPC request to produced an output in the Stream
/*!
 *  The RpcHandler is used to perform the type of processing on the socket:
 *  Json, Xml, MessagePack.
 *  The MethodManager contains the functions that are defined for the server.
 *  The request contains the data stream to parse for the type of request and parameters.
 *  The response will be placed in the Stream to send back to the client.
 */
typedef bool RpcHandler(MethodManager* manager, char* request, std::size_t length, Stream &response);

////////////////////////////////////////////////////////////////////////////////

//! Hold the information to match HTTP content-type field to an RpcHandler
/*!
 *  The class treats the requestContentType as a regular expression to match
 *  against the HTTP header content-type field.  This allows the match to not
 *  be exact to a single string.
 */
class RpcContentHandler
{
public:
    RpcContentHandler() : handler_(0), matchAnyContentType_(true) {}
    RpcContentHandler(RpcHandler* handler, std::string requestContentType, std::string responseContentType) :
        handler_(handler), requestContentType_(requestContentType), responseContentType_(responseContentType),
        matchAnyContentType_(requestContentType==""){}

    //! Perform processing on the request using this handler
    bool HandleRequest(MethodManager* manager, char* request, std::size_t length, Stream &response)
        { anyrpc_assert(handler_ != 0, AnyRpcErrorHandlerNotDefined, "The RPC handler was not defined");
          return handler_(manager,request,length,response); }
    //! Determine if this handler is able to process the given contentType
    bool CanProcessContentType(std::string contentType);
    //! Get the content-type string to use with the response
    std::string& GetResponseContentType() { return responseContentType_; }
    //! Set the field if you need to use a default constructor;
    void SetHandler(RpcHandler* handler, std::string requestContentType, std::string responseContentType)
        { handler_ = handler; requestContentType_ = requestContentType; responseContentType_ = responseContentType; }

private:
    log_define("AnyRPC.RpcHandler");

    RpcHandler* handler_;               //!< Function pointer to RPC handler
    bool matchAnyContentType_;          //!< Flag indicating that any content-type can be used
#if defined(ANYRPC_REGEX)
    std::regex requestContentType_;     //!< Regular express to match with the HTTP request content-type
#else
    std::string requestContentType_;    //!< String to match with the HTTP request content-type
#endif // #if defined(ANYRPC_REGEX)

    std::string responseContentType_;   //!< String to use in the HTTP response content-type field
};

//! A list of RpcContentHandlers that can be used to process a message
typedef std::vector<RpcContentHandler> RpcHandlerList;

////////////////////////////////////////////////////////////////////////////////

//! A connection from the server to a specific client.
/*!
 *  The connection can be processed in a thread or as a set of event driven
 *  by the server.
 *
 *  When using a thread, the connection will continue to process
 *  request until there is an unrecoverable error or the StopThread function is called.
 *
 *  When using the event handler, each call to process an event by HandleEvent
 *  will return with the types of events to look for.  If a zero is returned, then
 *  the connection will be closed.
 *
 *  The connection includes a limited buffer that is used for the header and short messages.
 *  If more buffer space is required, then this can be allocated when the header
 *  is processed.
 *
 *  The response is stored in a segmented buffer so that data copying is not required
 *  from realloc calls as a single buffer is expanded.
 */
class ANYRPC_API Connection
{
public:
    Connection(SOCKET fd, MethodManager* manager);
    virtual ~Connection();

    //! Initialize the data to start processing a new request
    virtual void Initialize(bool preserveBufferData=false);
    //! Get the file descriptor for the socket - needed for select calls
    virtual SOCKET GetFileDescriptor() { return socket_.GetFileDescriptor(); }
    //! Process data from the socket being either readable or writable.
    virtual void Process(bool executeAfterRead = true);
    //! Indicate that this connection should be closed.
    virtual void SetCloseState() { connectionState_ = CLOSE_CONNECTION; }
    //! Set the active flag - used for thread pool processing to determine whether the main thread should process this connection
    virtual void SetActive(bool active = true) { active_ = active; }

    //! Whether a select call should wait for readability of the socket
    virtual bool WaitForReadability() { return active_ && (connectionState_ <= READ_REQUEST); }
    //! Whether a select call should wait for writability of the socket
    virtual bool WaitForWritability() { return active_ && (connectionState_ == WRITE_RESPONSE); }
    //! Whether in the execute state - used for thread pool
    virtual bool CheckExecuteState() { return (connectionState_ == EXECUTE_REQUEST); }
    //! Whether the connection should be closed
    virtual bool CheckClose() { return (connectionState_ == CLOSE_CONNECTION); }
    //! Whether the connection can be forced to disconnect at this time
    virtual bool ForcedDisconnectAllowed() { return (bufferLength_ == 0); }
    //! Get the time when the last RPC transaction took place
    virtual time_t GetLastTransactionTime() { return lastTransactionTime_; }

#if defined(ANYRPC_THREADING)
    void StartThread();
    void StopThread(bool waitForJoin=true);
    bool IsThreadRunning() { return threadRunning_; }
#endif

protected:
    log_define("AnyRPC.Connection");

    //! Read the request header
    virtual bool ReadHeader() = 0;
    //! Read the request body
    virtual bool ReadRequest();
    //! Execute the request to produce the response
    virtual bool ExecuteRequest() = 0;
    //! Write the response - header and body
    virtual bool WriteResponse();

    TcpSocket socket_;                      //!< Socket for communication
    MethodManager *manager_;                //!< Pointer to the manager with the list of methods

    enum ConnectionState
    {
        READ_HEADER, READ_REQUEST, EXECUTE_REQUEST, WRITE_RESPONSE, CLOSE_CONNECTION
    };
    ConnectionState connectionState_;       //!< Current state for processing the RPC request
    time_t lastTransactionTime_;            //!< Time when the last transaction occurred - used to set priority for forced disconnect
    bool active_;

    static const std::size_t MaxBufferLength = 2048;
    static const std::size_t MaxContentLength = 1000000;

    char buffer_[MaxBufferLength+1];        //!< Fixed buffer for request header and possibly the body
    std::size_t bufferLength_;              //!< Amount of data in the buffer_
    std::size_t contentLength_;             //!< Number of bytes for the request body
    bool keepAlive_;                        //!< Indication that the connection should be keep alive after responding

    char* request_;                         //!< Pointer to the start of the request body
    std::size_t contentAvail_;              //!< Number of bytes of the request body in the buffer
    bool requestAllocated_;                 //!< Whether the request pointer is allocated or just a pointer to buffer_

    WriteSegmentedStream header_;           //!< Data for the response header
    std::size_t headerBytesWritten_;        //!< Number of bytes of the header already written

    WriteSegmentedStream response_;         //!< Data for the response body
    std::size_t resultBytesWritten_;        //!< Number of bytes of the body already written

#if defined(ANYRPC_THREADING)
private:
    //! Function that is called when the thread is started
    void ThreadStarter();
    //! Process data for a number of milliseconds before checking if the thread should stop
    void Work(int ms);

    std::thread thread_;                    //!< Thread information
    bool threadRunning_;                    //!< Indication that the thread should be running
#endif
};

////////////////////////////////////////////////////////////////////////////////

//! Process an HTTP server connection
/*!
 *  Provide the specifics necessary for processing as an HTTP server connection with
 *  reading the header, executing the request, and generating the response header.
 *  Only a limited amount of the HTTP header data is required.
 *  The content-length is processed to determine the size of the body.
 *  The keep alive info is processed for both HTTP/1.0 and HTTP/1.1 to determine
 *  whether to keep the connection open after the request is processed.
 */
class ANYRPC_API HttpConnection : public Connection
{
public:
    HttpConnection(SOCKET fd, MethodManager* manager, RpcHandlerList& handlers) :
        Connection(fd, manager), handlers_(handlers) {}

    virtual void Initialize(bool preserveBufferData=false);

protected:
    virtual bool ReadHeader();
    virtual bool ExecuteRequest();

private:
    void GeneratePOSTResponseHeader(std::size_t bodySize, std::string& contentType);
    void GenerateOPTIONSResponseHeader();
    void GenerateErrorResponseHeader(int code, std::string message);

    internal::HttpRequest httpRequestState_;    //!< Processing of the HTTP header
    RpcHandlerList& handlers_;                  //!< List of RPC handlers to check
};

////////////////////////////////////////////////////////////////////////////////

//! Process a TCP server connection using netstring protocol
/*!
 *  The netstring protocol uses following format:
 *  string's length written using ASCII digits, followed by a colon, the byte data, and a comma.
 *  "Length" in this context means "number of 8-bit units", so if the string is, for example,
 *  encoded using UTF-8, this may or may not be identical to the number of textual characters
 *  that are present in the string.
 */
class ANYRPC_API TcpConnection : public Connection
{
public:
    TcpConnection(SOCKET fd, MethodManager* manager, RpcHandler* handler) :
        Connection(fd, manager), handler_(handler), commaExpected_(false) {}

    virtual bool ForcedDisconnectAllowed() { return commaExpected_ ? (bufferLength_ <= 1) : (bufferLength_ == 0); }

protected:
    virtual bool ReadHeader();
    virtual bool ExecuteRequest();

private:
    RpcHandler *handler_;                   //!< Pointer to the handler to process the requests
    bool commaExpected_;                    //!< A comma separate is expected before the next message
};

} // namespace anyrpc

#endif // ANYRPC_CONNECTION_H_
