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

#ifndef ANYRPC_CLIENT_H_
#define ANYRPC_CLIENT_H_

#include "internal/http.h"

#if defined(__CYGWIN__)
# include <sys/time.h>
#endif

#if defined(ANYRPC_THREADING)
# include <atomic>
#endif  // defined(ANYRPC_THREADING)

namespace anyrpc
{

//! Return results from processing an RPC response
enum ProcessResponseEnum
{
    ProcessResponseSuccess,             //!< Successful processing
    ProcessResponseErrorKeepOpen,       //!< Error response but leave the connection open
    ProcessResponseErrorClose,          //!< Error response, close connection
};

//! Process the client information into a request using a specific protocol
/*!
 *  This is the base class for creating requests and processing the responses
 *  with a specific protocol: Json, Xmlrpc, MessagePack.
 *
 *  A static member is used to create a unique Id for each request if the protocol
 *  requires it.  There is currently no locking mechanism for atomic incrementing
 *  but this is not critical in the current implementation since unique Ids are
 *  not required.
 */
class ANYRPC_API ClientHandler
{
public:
    ClientHandler() {};
    virtual ~ClientHandler() {};

    //! Get the next unique id for protocols that require one
    static unsigned GetNextId();
    //! Generate the RPC request in the stream from the methods and parameters
    virtual bool GenerateRequest(const char* method, Value& params, Stream& os, unsigned& requestId, bool notification) = 0;
    //! Process the RPC response string.  The result will have any values returned.
    virtual ProcessResponseEnum ProcessResponse(char* response, std::size_t length, Value& result, unsigned requestId, bool notification) = 0;
    //! Generate a value result value with the code and message
    virtual void GenerateFaultResult(int errorCode, std::string const& msg, Value& result);

protected:
    log_define("AnyRPC.ClientHandler");

private:
#if defined(ANYRPC_THREADING)
    static std::atomic<unsigned> nextId_;    //!< Next id for protocols that require one - thread-safe
#else
    static unsigned nextId_;                 //!< Next id for protocols that require one
#endif // defined(ANYRPC_THREADING)
};

////////////////////////////////////////////////////////////////////////////////

//! Base class for RPC clients
/*!
 *  The Client class requires a handler for a specific protocol for the message
 *  and need to be specialized for a transport protocol.
 *
 *  Synchronous RPC is available with the Call function.
 *  Asynchronous RPC is available with the Post and GetPostResult functions.
 *  Multiple asynchronous RPC call can be made before getting the result since
 *  a list of the expected sequence is kept.
 *
 *  Notify calls are also available with the need for a response
 *  determined by the transport protocol.  A notification may also have
 *  a result from a delivery or protocol failure.
 *
 *  The client includes a limited buffer that is used for the response header and short messages.
 *  If more buffer space is required, then this can be allocated when the response header
 *  is processed.
 *
 *  The header and request are stored in a segmented buffer so that data copying is not required
 *  from realloc calls as a single buffer is expanded.
 */
class ANYRPC_API Client
{
public:
    Client(ClientHandler* handler);
    Client(ClientHandler* handler, const char* host, int port);
    virtual ~Client();

    //! Set the server name and port. This will close any currently active socket.
    virtual void SetServer(const char* host, int port) { host_ = host; port_ = port; Close(); }
    //! Set timeout for the client to respond to a request
    virtual void SetTimeout(unsigned msTime) { timeout_ = msTime; }
    //! Close the connection
    virtual void Close() { log_info("close socket, fd=" << socket_.GetFileDescriptor()); socket_.Close(); }

    virtual bool Call(const char* method, Value& params, Value& result);
    virtual bool Post(const char* method, Value& params, Value& result);
    virtual bool GetPostResult(Value& result);
    virtual bool Notify(const char* method, Value& params, Value& result);

    //! Start the client and connect to server
    virtual bool Start();
    //! Get ip and port of connection (local)
    virtual bool GetSockInfo(std::string& ip, unsigned& port) const { return socket_.GetSockInfo(ip, port); }
    //! Get ip and port of the host (remote)
    virtual bool GetPeerInfo(std::string& ip, unsigned& port) const { return socket_.GetPeerInfo(ip, port); }

protected:
    log_define("AnyRPC.Client");

    enum ProcessHeaderResult { HEADER_COMPLETE, HEADER_INCOMPLETE, HEADER_FAULT };

    //! Reset the connection, generally after a failure
    virtual void Reset();
    //! Reset the variable related to a transaction, generally before starting a new transaction
    virtual void ResetTransaction();
    //! Reset the receive buffer and discard any data
    virtual void ResetReceiveBuffer() { bufferLength_ = 0; }
    //! Reset the receive buffer but preserve any data that was not processed
    virtual void PreserveReceiveBuffer();
    //! Return the amount of time left for the call
    unsigned GetTimeLeft();
    //! Connect to the server
    virtual bool Connect(Value& result);
    //! Generate the RPC request into the request_ stream based on the method and params
    virtual bool GenerateRequest(const char* method, Value& params, bool notification=false);
    //! Generate the protocol specific header for the RPC request
    virtual bool GenerateHeader() = 0;
    //! Send the request to the server
    virtual bool WriteRequest(Value& result);
    //! Read back the RPC response header
    virtual bool ReadHeader(Value& result);
    //! Process the RPC response header, primarily to get the payload length
    virtual int ProcessHeader(bool /* eof */) { return HEADER_FAULT; }
    //! Read back the RPC response payload
    virtual bool ReadResponse(Value& result);
    //! Process the actual RPC response message
    virtual ProcessResponseEnum ProcessResponse(Value& result, bool notification=false);
    //! Indicate whether this protocol is expecting a response from a notification
    virtual bool TransportHasNotifyResponse() = 0;

    ClientHandler* handler_;                //!< Pointer to the handler to generate the request and process the response
    TcpSocket socket_;                      //!< Socket for communication

    WriteSegmentedStream header_;           //!< Data for the request header
    WriteSegmentedStream request_;          //!< Data for the request body
    std::list<unsigned> requestId_;         //!< Id for the last request

    static const std::size_t MaxBufferLength = 2048;
    static const std::size_t MaxContentLength = 1000000;

    char buffer_[MaxBufferLength+1];        //!< Fixed buffer for the response header and possibly the body
	std::size_t bufferLength_;              //!< Amount of data in the buffer_
	std::size_t contentLength_;             //!< Number of bytes for the response body

    char* response_;                        //!< Pointer to the start of the response body
	std::size_t contentAvail_;              //!< Number of bytes of the response in the buffer
    bool responseAllocated_;                //!< Whether the response pointer is allocated or just a pointer to buffer_

    struct timeval startTime_;              //!< Time that the request was started

    std::string host_;                      //!< Connection host name/IP address
    int port_;                              //!< Connection port
    unsigned timeout_;                      //!< Timeout value in milliseconds

    bool responseProcessed_;                //!< The response has been process and buffer needs to be reclaimed
};

////////////////////////////////////////////////////////////////////////////////

//! Process an HTTP client
/*!
 *  Provide the specifics necessary for processing as an HTTP client connection.
 *  The connection will use HTTP/1.1 and attempt to keep the connection open.
 */
class ANYRPC_API HttpClient : public Client
{
public:
    HttpClient(ClientHandler* handler, std::string contentType) :
        Client(handler), contentType_(contentType) {}

    HttpClient(ClientHandler* handler, std::string contentType, const char* host, int port) :
        Client(handler,host,port), contentType_(contentType) {}

    //! Reset for a new transaction including the HTTP header processing
    virtual void ResetTransaction() { Client::ResetTransaction(); httpResponseState_.Initialize(); }

protected:
    virtual bool GenerateHeader();
    virtual int ProcessHeader(bool eof);
    virtual ProcessResponseEnum ProcessResponse(Value& result, bool notification=false);
    virtual bool TransportHasNotifyResponse() { return true; }

    internal::HttpResponse httpResponseState_;  //!< Processing of the HTTP header
    std::string contentType_;
};

////////////////////////////////////////////////////////////////////////////////

//! Process a TCP client using netstring protocol
/*!
 *  The netstring protocol uses following format:
 *  string's length written using ASCII digits, followed by a colon, the byte data, and a comma.
 *  "Length" in this context means "number of 8-bit units", so if the string is, for example,
 *  encoded using UTF-8, this may or may not be identical to the number of textual characters
 *  that are present in the string.
 */
class ANYRPC_API TcpClient : public Client
{
public:
    TcpClient(ClientHandler* handler) : Client(handler), commaExpected_(false) {}

    TcpClient(ClientHandler* handler, const char* host, int port) :
        Client(handler,host,port), commaExpected_(false) {}

    // when closing the connection, you also don't expect the netstrings comma separator
    virtual void Close() { commaExpected_ = false; Client::Close(); }

protected:
    virtual bool GenerateHeader();
    virtual int ProcessHeader(bool eof);
    virtual bool TransportHasNotifyResponse() { return false; }

private:
    bool commaExpected_;        //!< Expecting the netstrings comma separator before the next message
};


} // namespace anyrpc

#endif // ANYRPC_CLIENT_H_
