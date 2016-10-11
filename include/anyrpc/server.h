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

#ifndef ANYRPC_SERVER_H_
#define ANYRPC_SERVER_H_

#if defined(ANYRPC_THREADING)
# if defined(__MINGW32__)
// These constants are not defined for mingw but are needed in the following libraries
#  ifndef EOWNERDEAD
#   define EOWNERDEAD       133    /* File too big */
#  endif
#  ifndef EPROTO
#   define EPROTO    134    /* Protocol error */
#  endif

#  include "internal/mingw.thread.h"
#  include <mutex>
#  include "internal/mingw.mutex.h"
#  include "internal/mingw.condition_variable.h"
# else
#  include <thread>
#  include <condition_variable>
#  include <mutex>
# endif //defined(__MINGW32__)
#endif //defined(ANYRPC_THREADING)

namespace anyrpc
{

//! Server for an RPC protocol
/*!
 *  The server can operate in four way.
 *  First, as part of the main thread by calling Work.
 *  Second, as an independent thread by calling StartThread.
 *  Third, as an independent thread with individual connection threads.
 *  Fourth, as an independent thread with multiple worker threads for execution.
 *
 *  The server's MethodManager must be populated with the available methods.
 *
 *  The BindAndListen call to set the port should be performed before
 *  calling the Work or StartThread functions.
 *
 *  There is a limit to the number of simultaneous connections.
 *  New connections will attempt to close an old connection with the assumption
 *  that the client will retry a failed attempt. This prevents stale
 *  connections from consuming resources. Set forcedDisconnectAllowed_ to false
 *  to disable this feature.
 */
class ANYRPC_API Server
{
public:
    Server();
    virtual ~Server() { Shutdown(); }

    //! Set the maximum number of simultaneous connections that the server can have
    void SetMaxConnections(unsigned maxConnections) { maxConnections_ = maxConnections; }
    //! Enable or Disable the closing of old connections on connection of new clients
    void SetForcedDisconnectAllowed(bool forcedDisconnectAllowed) { forcedDisconnectAllowed_ = forcedDisconnectAllowed; }
    //! Bind the server to a point and start listening for clients
    virtual bool BindAndListen(int port, int backlog = 5);
    //! Operate the server for a specified number of milliseconds
    virtual void Work(int ms) = 0;
    //! Close all of the connections
    virtual void Shutdown() { socket_.Close(); }
    //! Set the work loop to exit.  Also set the thread to exit if enabled.
    virtual void Exit();
    //! Get the method manager with the list of available methods
    MethodManager* GetMethodManager() { return &manager_; }
    //! Add a handler to the list of supported protocols - mostly for http servers
    void AddHandler(RpcHandler* handler, std::string requestContentType, std::string responseContentType);
    //! Get the list of handlers - mostly for http servers
    RpcHandlerList& GetRpcHandlerList() { return handlers_; }

#if defined(ANYRPC_THREADING)
    //! Start the server in a new thread
    virtual void StartThread();
    //! Stop the server and join the thread
    void StopThread();
#endif // defined(ANYRPC_THREADING)

    //! Get ip and port of the server (local)
    virtual bool GetMainSockInfo(std::string& ip, unsigned& port) const { return socket_.GetSockInfo(ip, port); }
    //! Get ip and port of sockets of all connections (local)
    virtual void GetConnectionsSockInfo(std::list<std::string>& ips, std::list<unsigned>& ports) const;
    //! Get ip and port of peers of all connections (remote)
    virtual void GetConnectionsPeerInfo(std::list<std::string>& ips, std::list<unsigned>& ports) const;

protected:
    log_define("AnyRPC.Server");

    //! Create a new connection using the supplied socket
    virtual Connection* CreateConnection(SOCKET fd) = 0;
    //! Add all of the protocol handlers to the list
    virtual void AddAllHandlers();

    TcpSocket socket_;             //!< Socket for communication
    int port_;                     //!< Port used for socket
    bool exit_;                    //!< Indication to exit the Work function or Thread
    bool working_;                 //!< Inside the work loop
    unsigned maxConnections_;      //!< Maximum number of simultaneous active connections
    bool forcedDisconnectAllowed_; //!< Allow disconnecting of inactive clients to free slots for new ones

    typedef std::list<Connection*> ConnectionList;
    ConnectionList connections_;   //!< List of active connections

#if defined(ANYRPC_THREADING)
    void ThreadStarter();

    std::thread thread_;           //!< Thread information
    bool threadRunning_;           //!< Indication that the thread should be running
#endif // defined(ANYRPC_THREADING)

private:
    MethodManager manager_;        //!< set of methods that can be called
    RpcHandlerList handlers_;      //!< set of protocols supported - mostly for http servers
};

////////////////////////////////////////////////////////////////////////////////

//! Server that uses a single thread for the server socket and all connection sockets
/*!
 *  The single threaded server monitors all of the sockets with a select call and
 *  then makes individual HandleEvent calls to the connections when they are ready.
 *  The HandleEvent call will return whether to continue monitoring the socket and
 *  whether to wait for readability or writability.
 *
 *  A server for a particular protocol will provide the CreateConnection function that
 *  will contain the connection protocol and the RPC handler to use.
 */
class ANYRPC_API ServerST : public Server
{
public:
    ServerST() {}

    virtual void Work(int ms);
    virtual void Shutdown();

protected:
    void AcceptConnection();
 };

////////////////////////////////////////////////////////////////////////////////

//! HTTP server that will process multiple protocols based on the content-type field of the header
class ANYRPC_API AnyHttpServer : public ServerST
{
public:
    AnyHttpServer() { AddAllHandlers(); }

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

#if defined(ANYRPC_THREADING)
//! Server that uses individual threads for each of the connections
/*!
 *  The multi-threaded server creates a new thread for each connection
 *  up to a maximum number of allowed threads.  When a new connection is
 *  accepted, it will either add to the list or look for a connection
 *  that has already been stopped.  If there are too many active connections
 *  then new connections are immediately closed.
  *
 *  A server for a particular protocol will provide the CreateConnection function that
 *  will contain the connection protocol and the RPC handler to use.
 */
class ANYRPC_API ServerMT : public Server
{
public:
    ServerMT() {}

    virtual void Work(int ms);
    virtual void Shutdown();

protected:
    void AcceptConnection();
};

////////////////////////////////////////////////////////////////////////////////

//! HTTP multi-threaded server that will process multiple protocols based on the content-type field of the header
class ANYRPC_API AnyHttpServerMT : public ServerMT
{
public:
    AnyHttpServerMT() { AddAllHandlers(); }

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }
};

////////////////////////////////////////////////////////////////////////////////

//! Server that uses a thread pool to execute the methods
/*!
 *  The thread-pool server creates a set of worker thread that are used to execute
 *  the methods that are called.  The main thread uses a select call to receive
 *  from all of the connections similar to ServerST but does not execute the method.
 *  The connection is then placed in a queue for the thread pool to perform the
 *  execution.  The worker can write the result and may continue with another message
 *  if it is already in the buffer.
 *
 *  The number of threads for the worker pool is separately defined from the maximum
 *  number of simultaneous connections.
 *
 *  The current implementation uses a UDP socket on the same port as the main server
 *  so that the worker thread can signal to the main thread that it is finished.
 *  This allows the server to add this thread to the select call.
 *  Under Linux this could be implemented with a signal and the pselect function,
 *  but this is not portable to Windows.
 *
 *  ServerTP should only be called by starting a thread and not by a direct call
 *  to Work although this is not prevented in the current implementation.
 */
class ANYRPC_API ServerTP : public ServerST
{
public:
    ServerTP() : numThreads_(4), workerExit_(false) {}
    ServerTP(const unsigned numThreads) : numThreads_(numThreads), workerExit_(false) {}

    virtual bool BindAndListen(int port, int backlog = 5);
    virtual void StartThread();
    virtual void Work(int ms);

private:
    void AcceptSignal();
    void ThreadStarter();
    void WorkerThread();

    unsigned numThreads_;                   //!< Number of worker threads
    std::vector<std::thread> workers_;      //!< List of worker threads

    std::queue<Connection*> workQueue_;     //!< Connections that are ready for the worker threads but not being processed
    std::mutex workQueueMutex_;             //!< Access mutex for the work queue
    std::condition_variable workerBlock_;   //!< Block for the worker threads waiting for work to do
    bool workerExit_;                       //!< Indication that the worker threads should exit
    UdpSocket serverSignal_;                //!< Signal to the main thread that a worker thread is dont with a connection
};

////////////////////////////////////////////////////////////////////////////////

//! HTTP thread-pool server that will process multiple protocols based on the content-type field of the header
class ANYRPC_API AnyHttpServerTP : public ServerTP
{
public:
    AnyHttpServerTP() { AddAllHandlers(); }
    AnyHttpServerTP(const unsigned numThreads) : ServerTP(numThreads) { AddAllHandlers(); }

protected:
    virtual Connection* CreateConnection(SOCKET fd) { return new HttpConnection(fd, GetMethodManager(), GetRpcHandlerList()); }

private:
};

#endif // defined(ANYRPC_THREADING)

} // namespace anyrpc

#endif // ANYRPC_SERVER_H_
