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

#ifndef ANYRPC_SOCKET_H_
#define ANYRPC_SOCKET_H_

#if !defined(WIN32)
typedef int SOCKET;     //!< SOCKET is used by Windows instead of int
#endif  // _WIN32

#if defined(__CYGWIN__)
# include <sys/select.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
#endif

namespace anyrpc
{

//! The Socket class implements generic access to a socket to hide implementation details across platforms.
/*!
 *  The socket file descriptor needs to be specified before operating on the socket.
 *
 *  On a socket error, the error code should be placed in the err_ variable since other calls may
 *  reset the value before an application can access it.
 */
class ANYRPC_API Socket
{
public:
    Socket();
    virtual ~Socket() { Close(); }

    void Close();

    void SetTimeout(unsigned timeout) { timeout_ = timeout; }

    int SetReuseAddress(int param=1);
    int SetKeepAlive(int param=1);
    int SetKeepAliveInterval(int startTime, int interval, int probeCount);
    int SetNonBlocking();

    SOCKET GetFileDescriptor() { return fd_; }
    void SetFileDescriptor(SOCKET fd) { fd_ = fd; }

    int Bind(int port);

    bool FatalError() { return FatalError(err_); }
    bool FatalError(int err);
    bool ConnectionResetError() { return ConnectionResetError(err_); }
    bool ConnectionResetError(int err);
    int GetLastError() { return err_; }

    bool WaitReadable(int timeout=-1);
    bool WaitWritable(int timeout=-1);

    //! Get information on ip and port of socket (local)
    virtual bool GetSockInfo(std::string& ip, unsigned& port) const;
    //! Get information on ip and port of peer (remote)
    virtual bool GetPeerInfo(std::string& ip, unsigned& port) const;

protected:
    void SetLastError();

    log_define("AnyRPC.Socket")

    SOCKET fd_;             //!< Socket file descriptor
    int err_;               //!< Error from last socket function
    int timeout_;           //!< Timeout for functions in milliseconds
};

////////////////////////////////////////////////////////////////////////////////

//! The TcpSock class builds on the Socket class with Tcp specific functions
/*!
 *  Many of the socket functions use a timeout.  The timeout can be either specified
 *  to the function or, if the timeout value is -1, default to a previously specified
 *  value from the SetTimeout function.
 *
 *  The file descriptor is automatically created when the TcpSocket is constructed.
 */
class ANYRPC_API TcpSocket : public Socket
{
public:
    TcpSocket() : connected_(false) { Create(); }

    SOCKET Create();
    int SetTcpNoDelay(int param=1);

    //! Send data on the socket.  The actual number of bytes written are returned in bytesWritten.
    bool Send(const char* str, std::size_t len, std::size_t &bytesWritten, int timeout=-1);
    //! Send data on the socket.  The actual number of bytes written are returned in bytesWritten.
    bool Send(std::string const& str, std::size_t bytesWritten, int timeout=-1)
        { return Send(str.c_str(), str.length(), bytesWritten, timeout); }

    //! Receive data from the socket.
    /*! The actual number of bytes received are returned in bytesRead.
     *  If the socket is detected as closed, then eof is set to true.
     */
    bool Receive(char* str, std::size_t maxLength, std::size_t &bytesRead, bool &eof, int timeout=-1);

    bool IsConnected(int timeout=-1);

    //! Used only by servers to listen on port after a bind
    int Listen(int backlog=5);
    //! Used only by servers to accept a connection
    SOCKET Accept();

    //! Used only by clients to connect to an IpAddress at a specified port
    int Connect(const char* ipAddress, int port);

protected:
    bool connected_;        //!< Connect function has been called
};

////////////////////////////////////////////////////////////////////////////////

//! The UdpSock class builds on the Socket class with Udp specific functions.
/*!
 *  The file descriptor is automatically created when the TcpSocket is constructed.
 *  This file descriptor will still need a bind call for server operations.
 */
class ANYRPC_API UdpSocket : public Socket
{
public:
    UdpSocket() { Create(); }

    SOCKET Create();

    //! Send data on the socket.  The actual number of bytes written are returned in bytesWritten.
    bool Send(const char* str, std::size_t len, std::size_t &bytesWritten, const char* ipAddress, int port);
    //! Send data on the socket.  The actual number of bytes written are returned in bytesWritten.
    bool Send(std::string const& str, std::size_t bytesWritten, std::string const& ipAddress, int port)
        { return Send(str.c_str(), str.length(), bytesWritten, ipAddress.c_str(), port); }

    //! Receive data from the socket.
    /*! The actual number of bytes received are returned in bytesRead.
     * If the socket is detected as closed, then eof is set to true.
     */
    bool Receive(char* str, int maxLength, int &bytesRead, bool &eof, std::string &ipAddress, int &port);
};

} // namespace anyrpc


#endif // ANYRPC_SOCKET_H_
