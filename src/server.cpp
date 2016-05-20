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
#include "anyrpc/internal/time.h"
#include "anyrpc/json/jsonserver.h"
#include "anyrpc/xml/xmlserver.h"
#include "anyrpc/messagepack/messagepackserver.h"

#ifndef WIN32
# include <unistd.h>   // for close()
#endif // WIN32

namespace anyrpc
{

Server::Server()
{
    log_trace();
    exit_ = false;
    working_ = false;
    maxConnections_ = 8;
    port_ = 0;

#if defined(ANYRPC_THREADING)
    threadRunning_ = false;
#endif // defined(ANYRPC_THREADING)
}

bool Server::BindAndListen(int port, int backlog)
{
    int result;

    port_ = port;

    // Don't block on reads/writes
    result = socket_.SetNonBlocking();
    if (result != 0)
    {
        socket_.Close();
        log_warn("Could not set socket to non-blocking input mode: " << result);
        return false;
    }

    // Allow this port to be re-bound immediately so server re-starts are not delayed
    result = socket_.SetReuseAddress();
    if (result != 0)
    {
        socket_.Close();
        log_warn("Could not set SO_REUSEADDR socket option: " << result);
        return false;
    }

    // Bind to the specified port on the default interface
    result = socket_.Bind(port);
    if (result != 0)
    {
        socket_.Close();
        log_warn("Could not bind to specified port : " << result);
        return false;
    }

    // Set in listening mode
    result = socket_.Listen(backlog);
    if (result != 0)
    {
        socket_.Close();
        log_warn("Could not set socket in listening mode: " << result);
        return false;
    }

    log_info("Server listening on port " << port << ", fd " << socket_.GetFileDescriptor());

    return true;
}

void Server::AddHandler(RpcHandler* handler, std::string requestContentType, std::string responseContentType)
{
    handlers_.push_back(RpcContentHandler(handler,requestContentType,responseContentType));
}

void Server::AddAllHandlers()
{
#if defined(ANYRPC_REGEX)
// Need c++11 compiler to support regular expression
# if defined(ANYRPC_INCLUDE_JSON)
    AddHandler( &JsonRpcHandler, "(.*)(json-rpc)", "application/json-rpc" );
# endif // defined(ANYRPC_INCLUDE_JSON)

# if defined(ANYRPC_INCLUDE_XML)
    AddHandler( &XmlRpcHandler, "(.*)(xml)", "text/xml" );
# endif // defined(ANYRPC_INCLUDE_XML)

# if defined(ANYRPC_INCLUDE_MESSAGEPACK)
    AddHandler( &MessagePackRpcHandler, "(.*)(messagepack-rpc)", "application/messagepack-rpc" );
# endif // defined(ANYRPC_INCLUDE_MESSAGEPACK)

#else
// Without c++11 compiler, use sub string find
# if defined(ANYRPC_INCLUDE_JSON)
    AddHandler( &JsonRpcHandler, "json-rpc", "application/json-rpc" );
# endif // defined(ANYRPC_INCLUDE_JSON)

# if defined(ANYRPC_INCLUDE_XML)
    AddHandler( &XmlRpcHandler, "xml", "text/xml" );
# endif // defined(ANYRPC_INCLUDE_XML)

# if defined(ANYRPC_INCLUDE_MESSAGEPACK)
    AddHandler( &MessagePackRpcHandler, "messagepack-rpc", "application/messagepack-rpc" );
# endif // defined(ANYRPC_INCLUDE_MESSAGEPACK)

#endif //defined(ANYRPC_REGEX)
}

void Server::Exit()
{
    exit_ = true;
#if defined(ANYRPC_THREADING)
    threadRunning_ = false;
#endif // defined(ANYRPC_THREADING)
}

#if defined(ANYRPC_THREADING)
void Server::StartThread()
{
    log_trace();
	threadRunning_ = true;
	thread_ = std::thread(&Server::ThreadStarter, this);
}

void Server::StopThread()
{
    log_trace();
    if (threadRunning_)
    {
        threadRunning_ = false;
        thread_.join();
    }
}

void Server::ThreadStarter()
{
	log_trace();
    while (threadRunning_ && !exit_)
    {
        Work(100);
    }
    Shutdown();
    threadRunning_ = false;
}
#endif // defined(ANYRPC_THREADING)

////////////////////////////////////////////////////////////////////////////////

void ServerST::Work(int ms)
{
    int timeLeft;
    struct timeval startTime;
    struct timeval currentTime;
    gettimeofday( &startTime, 0 );

    working_ = true;

    do
    {
        // Construct the sets of descriptors we are interested in
        fd_set inFd, outFd;
        FD_ZERO(&inFd);
        FD_ZERO(&outFd);

        // add server socket
        SOCKET maxFd = socket_.GetFileDescriptor();
        FD_SET(maxFd, &inFd);

        // add connection sockets
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); it++)
        {
            Connection* connection = *it;
            SOCKET fd = connection->GetFileDescriptor();
            if (connection->WaitForReadability())
            {
                FD_SET(fd, &inFd);
                maxFd = std::max(fd, maxFd);
            }
            if (connection->WaitForWritability())
            {
                FD_SET(fd, &outFd);
                maxFd = std::max(fd, maxFd);
            }
        }

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
        {
            break;
        }

        // Process server events
        if (FD_ISSET(socket_.GetFileDescriptor(), &inFd))
            AcceptConnection();

        // Process connection events
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end();)
        {
            // save this iterator position and move to the next so you can erase if needed
            Connection* connection = *it;
            SOCKET fd = connection->GetFileDescriptor();
            if (fd <= maxFd)
            {
                if ((FD_ISSET(fd, &inFd)) || (FD_ISSET(fd, &outFd)))
                {
                    try
                    {
                        connection->Process();
                    }
                    catch (AnyRpcException &fault)
                    {
                        // anyrpc exceptions shouldn't get to this point but attempt to handle gracefully
                        connection->SetCloseState();
                    }
                }
            }
            if (connection->CheckClose())
            {
                log_info("Stop monitoring fd=" << connection->GetFileDescriptor());
                delete connection;
                it = connections_.erase(it);
            }
            else
                it++;
        }

        gettimeofday( &currentTime, 0 );
        timeLeft = ms - MilliTimeDiff(currentTime,startTime);

    } while (!exit_ && ((ms < 0) || (timeLeft > 0)));

    working_ = false;
}

void ServerST::Shutdown()
{
    anyrpc_assert(!working_, AnyRpcErrorShutdown, "Illegal call to Shutdown");
    if (!working_)
    {
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); ++it)
            delete *it;
        connections_.clear();
    }
}

void ServerST::AcceptConnection()
{
    SOCKET fd = socket_.Accept();
    if (fd < 0)
    {
        log_warn("Could not accept connection, error=" << socket_.GetLastError());
        return;
    }
    // this should only loop through once unless maxConnections was changed while running
    while (connections_.size() >= maxConnections_)
    {
        // find connection that has been used the least recent
        ConnectionList::iterator targetIt = connections_.end();
        time_t targetTime = 0;
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); ++it)
        {
            Connection* connection = *it;
            if (connection->ForcedDisconnectAllowed())
            {
                time_t lastTime = connection->GetLastTransactionTime();
                if ((targetTime == 0) || (lastTime < targetTime))
                {
                    // keep a reference to this one
                    targetIt = it;
                    targetTime = lastTime;
                }
            }
        }
        if (targetIt == connections_.end())
        {
            // could not find one to disconnect
            log_debug( "Can't accept the connection, too many active connections" );
#ifdef WIN32
            closesocket(fd);
#else
            close(fd);
#endif // WIN32
            return;
        }
        log_debug("Force connection to close, fd=" << (*targetIt)->GetFileDescriptor());
        delete *targetIt;
        connections_.erase(targetIt);
    }
    // Listen for input on this source when we are in work()
    log_info("Creating a connection, fd=" << fd);
    Connection* connection = CreateConnection(fd);
    connections_.push_back( connection );
}

////////////////////////////////////////////////////////////////////////////////

#if defined(ANYRPC_THREADING)
void ServerMT::Work(int ms)
{
    int timeLeft;
    struct timeval startTime;
    struct timeval currentTime;
    gettimeofday( &startTime, 0 );

    working_ = true;

    do
    {
        // Construct the sets of descriptors we are interested in
        fd_set inFd;
        FD_ZERO(&inFd);

        // add server socket
        SOCKET maxFd = socket_.GetFileDescriptor();
        FD_SET(maxFd, &inFd);

        // Check for events
        int nEvents;
        if (ms < 0)
            nEvents = select(static_cast<int>(maxFd) + 1, &inFd, NULL, NULL, NULL);
        else
        {
            gettimeofday( &currentTime, 0 );
            timeLeft = ms - MilliTimeDiff(currentTime,startTime);

            struct timeval tv;
            tv.tv_sec = timeLeft / 1000;
            tv.tv_usec = (timeLeft - tv.tv_sec*1000) * 1000;
            nEvents = select(static_cast<int>(maxFd) + 1, &inFd, NULL, NULL, &tv);
        }

        if (nEvents < 0)
            break;

        // Process server events
        if (FD_ISSET(socket_.GetFileDescriptor(), &inFd))
            AcceptConnection();

        gettimeofday( &currentTime, 0 );
        timeLeft = ms - MilliTimeDiff(currentTime,startTime);

    } while (!exit_ && ((ms < 0) || (timeLeft > 0)));

    working_ = false;
}

void ServerMT::Shutdown()
{
    log_trace();
    // tell all connections to shutdown
    for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); ++it)
    {
        (*it)->StopThread();
        delete *it;
    }

    connections_.clear();
}

void ServerMT::AcceptConnection()
{
    log_trace();
    SOCKET fd = socket_.Accept();
    if (fd < 0)
    {
        log_warn("Could not accept connection, error=" << socket_.GetLastError());
        return;
    }
    // check if any of the threads have stopped running
    for (ConnectionList::iterator it = connections_.begin(); it != connections_.end();)
    {
        if (!(*it)->IsThreadRunning())
        {
            log_debug("Thread has already stopped");
            // not running but this will call join
            (*it)->StopThread();
            delete *it;
            it = connections_.erase(it);
        }
        else
            it++;
    }
    // check if any can be forced to disconnect
    if (connections_.size() >= maxConnections_)
    {
        // find connection that has been used the least recent
        ConnectionList::iterator targetIt = connections_.end();
        time_t targetTime = 0;
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); ++it)
        {
            Connection* connection = *it;
            if (connection->ForcedDisconnectAllowed())
            {
                time_t lastTime = connection->GetLastTransactionTime();
                if ((targetTime == 0) || (lastTime < targetTime))
                {
                    // keep a reference to this one
                    targetIt = it;
                    targetTime = lastTime;
                }
            }
        }
        if (targetIt != connections_.end())
        {
            // indicate that this connection should close and then wait for it
            Connection* connection = *targetIt;
            log_debug("Force connection to close, fd=" << connection->GetFileDescriptor());
            connection->StopThread(false);
            unsigned delay = 5;
            for (unsigned i=0; i<(200/delay); i++)
            {
                if (!connection->IsThreadRunning())
                    break;
                MilliSleep(delay);
            }
            // check if it is still running
            if (!connection->IsThreadRunning())
            {
                connection->StopThread();
                delete *targetIt;
                connections_.erase(targetIt);
            }
        }
    }
    if (connections_.size() >= maxConnections_)
    {
        log_debug( "Can't accept the connection, too many active connections" );
#ifdef WIN32
        closesocket(fd);
#else
        close(fd);
#endif // WIN32
    }
    else
    {
        log_info("Creating a connection: " << fd);
        Connection* connection = CreateConnection(fd);
        connections_.push_back(connection);
        connection->StartThread();
    }
}

////////////////////////////////////////////////////////////////////////////////

void ServerTP::StartThread()
{
    log_trace();
    threadRunning_ = true;
    thread_ = std::thread(&ServerTP::ThreadStarter, this);
}

void ServerTP::ThreadStarter()
{
    log_trace();

    // start the worker threads
    workerExit_ = false;
    for (unsigned i=0; i<numThreads_; i++)
        workers_.emplace_back( &ServerTP::WorkerThread, this );

    // run the system
    while (threadRunning_ && !exit_)
    {
        Work(100);
    }

    // stop the worker threads
    workerExit_ = true;
    workerBlock_.notify_all();
    for (std::thread &worker: workers_)
        worker.join();

    // shutdown the rest of the system
    Shutdown();
    threadRunning_ = false;
}

////////////////////////////////////////////////////////////////////////////////

bool ServerTP::BindAndListen(int port, int backlog)
{
    int result;

    // Setup the signal socket as UDP on the same port as the main socket

    // Don't block on reads/writes
    result = serverSignal_.SetNonBlocking();
    if (result != 0)
    {
        serverSignal_.Close();
        log_warn("Could not set socket to non-blocking input mode: " << result);
        return false;
    }

    // Allow this port to be re-bound immediately so server re-starts are not delayed
    result = serverSignal_.SetReuseAddress();
    if (result != 0)
    {
        serverSignal_.Close();
        log_warn("Could not set SO_REUSEADDR socket option: " << result);
        return false;
    }

    // Bind to the specified port on the default interface
    result = serverSignal_.Bind(port);
    if (result != 0)
    {
        serverSignal_.Close();
        log_warn("Could not bind to specified port : " << result);
        return false;
    }

    return Server::BindAndListen(port, backlog);
}

void ServerTP::Work(int ms)
{
    int timeLeft;
    struct timeval startTime;
    struct timeval currentTime;
    gettimeofday( &startTime, 0 );

    working_ = true;

    do
    {
        // Construct the sets of descriptors we are interested in
        fd_set inFd, outFd;
        FD_ZERO(&inFd);
        FD_ZERO(&outFd);

        // add server socket
        SOCKET maxFd = socket_.GetFileDescriptor();
        FD_SET(maxFd, &inFd);

        // add signal socket
        SOCKET fd = serverSignal_.GetFileDescriptor();
        maxFd = std::max(fd, maxFd);
        FD_SET(fd, &inFd);

        // add connection sockets
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); it++)
        {
            Connection* connection = *it;
            fd = connection->GetFileDescriptor();
            if (connection->WaitForReadability())
            {
                log_debug("Wait for readability, fd=" << fd);
                FD_SET(fd, &inFd);
                maxFd = std::max(fd, maxFd);
            }
            if (connection->WaitForWritability())
            {
                log_debug("Wait for writability, fd=" << fd);
                FD_SET(fd, &outFd);
                maxFd = std::max(fd, maxFd);
            }
        }

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
        {
            break;
        }

        // Process server events
        if (FD_ISSET(socket_.GetFileDescriptor(), &inFd))
            AcceptConnection();

        if (FD_ISSET(serverSignal_.GetFileDescriptor(), &inFd))
            AcceptSignal();

        // Process connection events
        for (ConnectionList::iterator it = connections_.begin(); it != connections_.end();)
        {
            // save this iterator position and move to the next so you can erase if needed
            Connection* connection = *it;
            fd = connection->GetFileDescriptor();
            if (fd <= maxFd)
            {
                if ((FD_ISSET(fd, &inFd)) || (FD_ISSET(fd, &outFd)))
                {
                    // process the message but only until the execute stage
                    try
                    {
                        connection->Process( false );
                    }
                    catch (AnyRpcException &fault)
                    {
                        // anyrpc exceptions shouldn't get to this point but attempt to handle gracefully
                        connection->SetCloseState();
                    }
                    if (connection->CheckExecuteState())
                    {
                        // add the connection to the queue for the thread pool
                        log_info("Send connection to thread pool, fd=" << connection->GetFileDescriptor());
                        // used to indicate that the connection should not be part of the main thread select
                        connection->SetActive(false);
                        // add to the work queue and signal a worker
                        std::unique_lock<std::mutex> lock(workQueueMutex_);
                        workQueue_.push(connection);
                        workerBlock_.notify_one();
                    }
                }
            }
            if (connection->CheckClose())
            {
                // this connection has closed so removed from monitoring and delete
                log_info("Stop monitoring fd=" << connection->GetFileDescriptor());
                delete connection;
                // delete from list and move the iterator to the next value
                it = connections_.erase(it);
            }
            else
                // move the iterator to the next value
                it++;
        }

        gettimeofday( &currentTime, 0 );
        timeLeft = ms - MilliTimeDiff(currentTime,startTime);

    } while (!exit_ && ((ms < 0) || (timeLeft > 0)));

    working_ = false;
}

void ServerTP::AcceptSignal()
{
    char buffer[256];
    int bytesRead;
    bool eof;
    std::string ipAddress;
    int port;

    // Read the message out although you don't really need the information
    serverSignal_.Receive(buffer, sizeof(buffer), bytesRead, eof, ipAddress, port);
    log_info("Accept signal");
}

void ServerTP::WorkerThread()
{
    log_trace();

    UdpSocket signal;
    char buffer = 0;
    size_t bytesWritten;
    const char* ipAddress = "127.0.0.1";

    while (true)
    {
        std::unique_lock<std::mutex> lock(workQueueMutex_);

        // wait until signaled that there is work or need to exit
        workerBlock_.wait(lock, [this]{ return this->workerExit_ || !this->workQueue_.empty();});

        // check if worker should exit - don't worry about whether the work queue is empty
        if (workerExit_)
            return;

        // get the next item from the work queue and release the lock
        Connection* connection = workQueue_.front();
        workQueue_.pop();
        lock.unlock();

        // process the connection method and continue until it will block
        log_info("Process from thread pool, fd=" << connection->GetFileDescriptor());
        try
        {
            connection->Process();
        }
        catch (AnyRpcException &fault)
        {
            // anyrpc exceptions shouldn't get to this point but attempt to handle gracefully
            connection->SetCloseState();
        }

        // return the connection to active status so it will be processed by the main thread
        log_info("Finished with connection from thread pool, fd=" << connection->GetFileDescriptor());
        connection->SetActive();

        // send signal to main thread so that it will interrupt the select call
        log_info("Send signal");
        signal.Send(&buffer, 1, bytesWritten, ipAddress, port_);
    }
}

#endif // defined(ANYRPC_THREADING)

} // namespace anyrpc
