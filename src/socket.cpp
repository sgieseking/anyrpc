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
#include "anyrpc/socket.h"
#include "anyrpc/internal/time.h"

#if defined(_MSC_VER)
# pragma warning(disable:4996)	// inet_addr is marked as deprecated
#endif // _MSC_VER

#if !defined(WIN32)
extern "C"
{
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <errno.h>
# include <fcntl.h>
}
#endif  // _WIN32

namespace anyrpc
{

#if defined(WIN32)

log_define("");

//! Start up winsock with the first socket
static void InitWinSock()
{
    static bool wsInit = false;
    if (! wsInit)
    {
        WORD wVersionRequested = MAKEWORD( 2, 0 );
        WSADATA wsaData;
        int err = WSAStartup(wVersionRequested, &wsaData);
        wsInit = true;
        log_fatal_if( (err != 0), "WSAStartup = " << err);
    }
}
#endif // WIN32

Socket::Socket()
{
#if defined(WIN32)
    InitWinSock();
#endif
    fd_ = static_cast<SOCKET>(-1);
    timeout_ = 0;
    err_ = 0;
}

void Socket::Close()
{
#ifdef WIN32
    closesocket(fd_);
#else
    close(fd_);
#endif // WIN32
    fd_ = static_cast<SOCKET>(-1);
}

int Socket::SetReuseAddress(int param)
{
    int result = setsockopt( fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&param, sizeof(param) );
    log_debug( "SetReuseAddress: param=" << param << ", result=" << result);
    return result;
}

int Socket::SetKeepAlive(int param)
{
    int result = setsockopt( fd_, SOL_SOCKET, SO_KEEPALIVE, (char*)&param, sizeof(param) );
    log_debug( "SetKeepAlive: param=" << param << ", result=" << result);
    return result;
}

int Socket::SetKeepAliveInterval(int startTime, int interval, int probeCount)
{
    log_debug( "SetKeepAliveInterval: startTime=" << startTime << ", interval=" << interval << ", probeCount=" << probeCount);
#if defined(_MSC_VER)
    DWORD outBytes;
    tcp_keepalive tcp_ka;
    tcp_ka.onoff = 1;
    tcp_ka.keepalivetime = startTime * 1000;
    tcp_ka.keepaliveinterval = interval * 1000;

    int result = WSAIoctl(fd_, SIO_KEEPALIVE_VALS, &tcp_ka, sizeof(tcp_ka), NULL, 0, &outBytes, NULL, NULL);
    if (result < 0)
        log_debug( "SetKeepAliveInterval: result = " << result );
    return result;
#elif defined(__MINGW32__)
    // don't see how this can be performed right now
#elif (__APPLE__)
    int result = setsockopt( fd_, IPPROTO_TCP, TCP_KEEPALIVE, (char*)&startTime, sizeof(startTime) );
    if (result < 0)
    {
        log_debug( "SetKeepAliveInterval: set keep idle result = " << result );
    }
    return result;
#else
    int result = setsockopt( fd_, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&startTime, sizeof(startTime) );
    if (result < 0)
    {
        log_debug( "SetKeepAliveInterval: set keep idle result = " << result );
        return result;
    }
    result = setsockopt( fd_, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&interval, sizeof(interval) );
    if (result < 0)
    {
        log_debug( "SetKeepAliveInterval: set interval result = " << result );
        return result;
    }
    result = setsockopt( fd_, IPPROTO_TCP, TCP_KEEPCNT, (char*)&probeCount, sizeof(probeCount) );
    if (result < 0)
        log_debug( "SetKeepAliveInterval: set probe count result = " << result );
    return result;
#endif
}

int Socket::SetNonBlocking()
{
    int result;
#if defined(WIN32)
    unsigned long flag = 1;
    result = ioctlsocket(fd_, FIONBIO, &flag);
#else
    result = fcntl(fd_, F_SETFL, O_NONBLOCK);
#endif // WIN32
    log_debug( "SetNonBlocking: result=" << result);
    return result;
}

int Socket::Bind( int port )
{
    struct sockaddr_in sockAddress;

    memset( &sockAddress, 0, sizeof(sockAddress) );
    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = htons( port );
    sockAddress.sin_addr.s_addr = INADDR_ANY;

    int result = bind(fd_, (struct sockaddr*)&sockAddress, sizeof(sockAddress));
    SetLastError();  // the logging system may reset errno in Linux
    log_debug("Bind: port=" << port << ", result=" << result << ", err=" << err_);
    return result;
}

bool Socket::FatalError(int err)
{
#ifdef WIN32
    return (err != WSAEINPROGRESS) && (err != EAGAIN) && (err != WSAEWOULDBLOCK) && (err != EINTR);
#else
    return (err != EINPROGRESS) && (err != EAGAIN) && (err != EWOULDBLOCK) && (err != EINTR);
#endif
}

bool Socket::ConnectionResetError(int err)
{
#ifdef WIN32
    return (err == WSAECONNRESET);
#else
    return (err == ECONNRESET);
#endif
}

void Socket::SetLastError()
{
#ifdef WIN32
    err_ = WSAGetLastError();
#else
    err_ = errno;
#endif
}

bool Socket::WaitReadable(int timeout)
{
    if (fd_ < 0)
    {
        log_warn("File descriptor not defined");
        return false;
    }

    if (timeout < 0) timeout = timeout_;    // default to the set timeout value
    timeout = std::max(0,timeout);          // timeout can't be negative

    struct timeval tval;
    tval.tv_sec = timeout/1000;
    tval.tv_usec = (timeout % 1000) * 1000;

    fd_set readfds;
    FD_ZERO( &readfds );
    FD_SET( fd_, &readfds );

    // check for readability
    int selectResult = select( static_cast<int>(fd_) + 1, &readfds, 0, 0, &tval );
    if (selectResult <= 0)
    {
        log_debug("WaitReadable: Select result=" << selectResult);
        return false;
    }

    log_debug("WaitReadable success");
    return true;
}

bool Socket::WaitWritable(int timeout)
{
    if (fd_ < 0)
    {
        log_warn("File descriptor not defined");
        return false;
    }

    if (timeout < 0) timeout = timeout_;    // default to the set timeout value
    timeout = std::max(0,timeout);          // timeout can't be negative

    struct timeval tval;
    tval.tv_sec = timeout/1000;
    tval.tv_usec = (timeout % 1000) * 1000;

    fd_set writefds;
    FD_ZERO( &writefds );
    FD_SET( fd_, &writefds );

    // check for readability
    int selectResult = select( static_cast<int>(fd_) + 1, 0, &writefds, 0, &tval );
    if (selectResult <= 0)
    {
        log_debug("WaitWritable: Select result=" << selectResult);
        return false;
    }

    log_debug("WaitWritable success");
    return true;
}

////////////////////////////////////////////////////////////////////////////////

SOCKET TcpSocket::Create()
{
    fd_ = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    connected_ = false;
    log_debug( "Create: fd=" << fd_);
    return fd_;
}

int TcpSocket::SetTcpNoDelay(int param)
{
    int result = setsockopt( fd_, IPPROTO_TCP, TCP_NODELAY, (char*)&param, sizeof(param) );
    log_debug( "SetTcpNoDelay: param=" << param << ", result=" << result);
    return result;
}

bool TcpSocket::Send(const char* buffer, size_t length, size_t &bytesWritten, int timeout)
{
    log_debug("Send: length=" << length << ", buffer=" << buffer);

    if (timeout < 0) timeout = timeout_;    // default to the set timeout value
    timeout = std::max(0,timeout);          // timeout can't be negative

    bytesWritten = 0;
    struct timeval startTime;
    gettimeofday( &startTime, 0 );
    while (true)
    {
#ifdef MSG_NOSIGNAL
        int numBytes = send( fd_, buffer+bytesWritten, static_cast<int>(length-bytesWritten),  MSG_NOSIGNAL);
#else
        int numBytes = send( fd_, buffer+bytesWritten, static_cast<int>(length-bytesWritten),  SO_NOSIGPIPE);
#endif
        SetLastError();
        log_debug("Send: numBytes=" << numBytes << ", err=" << err_);
        if (numBytes < 0)
        {
            if (FatalError())
                return false;
        }
        else
        {
            bytesWritten += numBytes;
            if (bytesWritten >= length)
                return true;
        }
        struct timeval currentTime;
        gettimeofday( &currentTime, 0 );
        int timeLeft = timeout - MilliTimeDiff(currentTime,startTime);
        if (timeLeft <= 0)
            break;
        // wait until space to write more data
        if (!WaitWritable(timeLeft))
            break;
    }
    // user timeout condition
    err_ = EAGAIN;
    return false;
}

bool TcpSocket::Receive(char* buffer, size_t maxLength, size_t &bytesRead, bool &eof, int timeout)
{
    if (timeout < 0) timeout = timeout_;    // default to the set timeout value
    timeout = std::max(0,timeout);          // timeout can't be negative

    struct timeval startTime;
    gettimeofday( &startTime, 0 );
    bytesRead = 0;
    eof = false;
    while (bytesRead < maxLength)
    {
        int numBytes = recv( fd_, buffer+bytesRead, static_cast<int>(maxLength-bytesRead), 0 );
        SetLastError();  // the logging system may reset errno in Linux
        log_debug( "Receive: numBytes=" << numBytes << ", err=" << err_);

        if (numBytes <= 0)
        {
            // a connection reset is also considered an EOF event
            // a reset may be generated by a client that closes the socket instead of using shutdown then close
            //   but this depends on the platform and the sequence of events
            eof = (numBytes == 0) || ConnectionResetError(err_);
            if (eof || FatalError())
                return false;
        }
        else
        {
            bytesRead += numBytes;
            buffer[bytesRead] = 0;
            log_debug( "Receive: data=" << buffer);
            if (bytesRead >= maxLength)
                return true;
        }
        struct timeval currentTime;
        gettimeofday( &currentTime, 0 );
        int timeLeft = timeout - MilliTimeDiff(currentTime,startTime);
        if (timeLeft <= 0)
            break;
        // wait until more data is available
        if (!WaitReadable(timeLeft))
            break;
    }
    // user timeout condition
    err_ = EAGAIN;
    return true;
}

bool TcpSocket::IsConnected(int timeout)
{
    if (fd_ < 0)
    {
        log_debug("File descriptor not defined");
        return false;
    }
    if (!connected_)
    {
        log_debug("Connect function has not been called");
        return false;
    }

    if (timeout < 0) timeout = timeout_;    // default to the set timeout value
    timeout = std::max(0,timeout);          // timeout can't be negative

    struct timeval tval;
    tval.tv_sec = timeout/1000;
    tval.tv_usec = (timeout % 1000) * 1000;

    fd_set writefds;
    FD_ZERO( &writefds );
    FD_SET( fd_, &writefds );

    // check for writability
    int selectResult = select( static_cast<int>(fd_) + 1, 0, &writefds, 0, &tval );
    if (selectResult <= 0)
    {
        log_warn("IsConnected failed: Select result=" << selectResult);
        return false;
    }

    // check for socket connection without error
    int optVal;
    int optLen = sizeof(optVal);
    int getsockoptResult = getsockopt( fd_, SOL_SOCKET, SO_ERROR, (char*)&optVal, (socklen_t*)&optLen );
    if ((getsockoptResult < 0) || (optVal != 0))
    {
        log_warn("IsConnected failed: OptVal=" << optVal << ", result=" << getsockoptResult);
        return false;
    }

    log_debug("IsConnected success");
    return true;
}

int TcpSocket::Listen(int backlog)
{
    int result = listen( fd_, backlog );
    log_debug("Listen: result=" << result);
    return result;
}

SOCKET TcpSocket::Accept()
{
    SOCKET result = accept( fd_, 0, 0 );
    log_debug("Accept: result=" << result);
    SetLastError();
    return result;
}

int TcpSocket::Connect(const char* ipAddress, int port)
{
    if (fd_ < 0)
        Create();

    struct sockaddr_in connectedAddr;
    memset( &connectedAddr, 0, sizeof(connectedAddr) );

    connectedAddr.sin_family = AF_INET;
    connectedAddr.sin_port = htons( port );
    connectedAddr.sin_addr.s_addr = inet_addr( ipAddress );

    int result = connect(fd_, (sockaddr*)&connectedAddr, sizeof(connectedAddr));
    SetLastError();
    connected_ = true;
    log_debug("Connect: ipAddress=" << ipAddress << ", port=" << port << ", result=" << result << ", err=" << err_);
    return result;
}

////////////////////////////////////////////////////////////////////////////////

SOCKET UdpSocket::Create()
{
    fd_ = socket( AF_INET, SOCK_DGRAM, 0 );
    log_debug( "Create: fd=" << fd_);
    return fd_;
}

bool UdpSocket::Send(const char* buffer, std::size_t length, std::size_t &bytesWritten, const char* ipAddress, int port)
{
    log_debug("Send: ipAddress=" << ipAddress << ", port=" << port << ", length=" << length << ", buffer=" << buffer);

    struct sockaddr_in sendAddr;
    memset( &sendAddr, 0, sizeof(sendAddr) );

    sendAddr.sin_family = AF_INET;
    sendAddr.sin_port = htons( port );
    sendAddr.sin_addr.s_addr = inet_addr( ipAddress );

    bytesWritten = 0;
    struct timeval startTime;
    gettimeofday( &startTime, 0 );

#ifdef MSG_NOSIGNAL
    int numBytes = sendto( fd_, buffer+bytesWritten, static_cast<int>(length-bytesWritten), MSG_NOSIGNAL,
                            (struct sockaddr *)&sendAddr, sizeof(sendAddr) );
#else
    int numBytes = sendto( fd_, buffer+bytesWritten, static_cast<int>(length-bytesWritten), SO_NOSIGPIPE,
                            (struct sockaddr *)&sendAddr, sizeof(sendAddr) );
#endif

    SetLastError();
    log_debug("Send: numBytes=" << numBytes << ", err=" << err_);
    if (numBytes < 0)
    {
        if (FatalError())
            return false;
    }
    else
    {
        bytesWritten += numBytes;
        if (bytesWritten >= length)
            return true;
    }

    // user timeout condition
    err_ = EAGAIN;
    return false;
}

bool UdpSocket::Receive(char* buffer, int maxLength, int &bytesRead, bool &eof, std::string &ipAddress, int &port)
{
    struct sockaddr_in receiveAddr;
    socklen_t receiveAddrLength = sizeof(receiveAddr);

    bytesRead = 0;
    eof = false;

    int numBytes = recvfrom( fd_, buffer+bytesRead, maxLength-bytesRead, MSG_DONTWAIT,
                             (struct sockaddr*)&receiveAddr, &receiveAddrLength);
    SetLastError();  // the logging system may reset errno in Linux
    log_debug( "Receive: numBytes=" << numBytes << ", err=" << err_);

    port = ntohs(receiveAddr.sin_port);

#if defined(__MINGW32__)
    // should be thread-safe since it would use the Windows call
    ipAddress = inet_ntoa(receiveAddr.sin_addr);
#else
    // Only need this buffer to perform the address conversion in a thread-safe call
    const unsigned bufferLength = 100;
    char addrBuffer[bufferLength];
    ipAddress = inet_ntop(AF_INET,&receiveAddr.sin_addr, addrBuffer, bufferLength);
#endif
    log_debug("Udp Receive: address=" << ipAddress << ", port=" << port);

    eof = (numBytes == 0);
    if (numBytes <= 0)
    {
        if (FatalError())
            return false;
    }
    else
    {
        bytesRead += numBytes;
        buffer[bytesRead] = 0;
        log_debug( "Receive: data=" << buffer);
        if (bytesRead >= maxLength)
            return true;
    }

    // user timeout condition - this may not be relevant for a UDP receive
    err_ = EAGAIN;
    return true;
}

} // namespace anyrpc
