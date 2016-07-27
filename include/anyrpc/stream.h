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

#ifndef ANYRPC_STREAM_H_
#define ANYRPC_STREAM_H_

#include "api.h"

namespace anyrpc
{

//! The Stream class is the base class used to read and write data from files, strings, buffers, etc.
//! and is modeled on the rapidJson interface with certain changes.
//! The base Stream class can not be used directly.

class ANYRPC_API Stream
{
public:
    Stream() {}
    virtual ~Stream() {}

    //! Processing of strings and binary data can be performed directly on the stream (i.e. it can be modified)
    virtual bool InSitu() const { return false; }
    //! Strings and binary data should be copy instead of referenced.
    virtual bool UseStringCopy() const { return true; }

    //! End of file has been reached
    virtual bool Eof() const { return true; }
    //! Return the next character but do not move forward
    virtual char Peek() const { anyrpc_assert(false,AnyRpcErrorIllegalCall,"Illegal call"); return 0; }
    //! Return the next character and move forward
    virtual char Get() { anyrpc_assert(false,AnyRpcErrorIllegalCall,"Illegal call"); return 0; }
    //! Return the next character, set the data to 0, and move forward.  Sometimes used with InSitu processing.
    virtual char GetClear() { return Get(); }
    //! Read up to length characters.  Return the number of characters read.
    virtual std::size_t Read(char* , std::size_t ) { anyrpc_assert(false,AnyRpcErrorIllegalCall,"Illegal call"); return 0; }
    //! Skip forward up to length characters.  Return the number of characters skipped.
    virtual std::size_t Skip(std::size_t ) { anyrpc_assert(false,AnyRpcErrorIllegalCall,"Illegal call"); return 0; }

    //! Mark the current position as the destination.  Used with InSitu processing.
    virtual char* PutBegin() { anyrpc_assert(false,AnyRpcErrorIllegalCall,"Illegal call"); return 0; }
    //! Put a character in the stream
    virtual void Put(char) {}
    //! Put a string in the stream.  Must be null terminated.
    virtual void Put(const char *) {}
    //! Put a std:string in the stream.
    virtual void Put(const std::string &) {}
    //! Put a length of characters in the stream.
    virtual void Put(const char *, std::size_t ) {}
    //! Reset the mark for putting characters in the stream with InSitu processing.
    virtual std::size_t PutEnd() { anyrpc_assert(false,AnyRpcErrorIllegalCall,"Illegal call"); return 0; }

    //! Return the current character position in a read stream. Used to indicate the position of an error.
    virtual std::size_t Tell() const { return 0; }
    //! Flush any buffered data out of the stream and to through a file.
    virtual void Flush() {}

protected:
    log_define("AnyRPC.Stream");
};

////////////////////////////////////////////////////////////////////////////////

//! StdOutStream provides a more direct way to send data to stdout without
//! needing to use WriteFileStream.
//! This class does not buffer any data so there is no need to flush the stream
//! at the end.
class ANYRPC_API StdOutStream : public Stream
{
public:
    virtual void Put(char c) { std::cout << c; }
    virtual void Put(const char *str) { std::cout << str; }
    virtual void Put(const std::string &str) { std::cout << str; }
    virtual void Put(const char *str, std::size_t n) { fwrite(str, 1, n, stdout); }
};

//! Static stream that can be used directly by an application.
static StdOutStream stdoutStream;

////////////////////////////////////////////////////////////////////////////////

const int DefaultFileBufferSize = 1024;

//! FileStream is the base class for buffered file access.
//! The buffering improves the performance compared with accessing the file directly
//! for each operation.

class ANYRPC_API FileStream : public Stream
{
public:
    FileStream(std::size_t bufferSize=DefaultFileBufferSize);
    virtual ~FileStream() { Close(); delete[] buffer_; }

    virtual void SetFilePointer(std::FILE* fp) { fp_ = fp; }
    virtual std::FILE* GetFilePointer(bool resetFilePointer=false);
    virtual void Close();

protected:
    std::FILE* fp_;
    char* buffer_;
    std::size_t bufferSize_;
};

////////////////////////////////////////////////////////////////////////////////

//! WriteFileStream is used to write to a file using buffering.
//! If a filename is given to the constructor, then that file is opened.
//! If a file pointer is given, then is should have been opened already.
//! The constructor can also be used without a file pointer with the assumption
//! that a SetFilePointer call will be made before any data is written.

class ANYRPC_API WriteFileStream : public FileStream
{
public:
    WriteFileStream(const char* filename, std::size_t bufferSize=DefaultFileBufferSize);
    WriteFileStream(std::FILE* fp, std::size_t bufferSize=DefaultFileBufferSize);
    WriteFileStream(std::size_t bufferSize=DefaultFileBufferSize);

    virtual void Put(char c);
    virtual void Put(const char *str) { Put(str,strlen(str)); }
    virtual void Put(const std::string &str) { Put(str.c_str(),str.length()); }
    virtual void Put(const char *str, std::size_t n);

    virtual void Flush();

private:
    char* bufferLast_;
    char* current_;
};

////////////////////////////////////////////////////////////////////////////////

//! ReadFileStream is used to read from a file using buffering.
//! If a filename is given to the constructor, then that file is opened.
//! If a file pointer is given, then is should have been opened already.
//! The constructor can also be used without a file pointer with the assumption
//! that a SetFilePointer call will be made before any data is read.

class ANYRPC_API ReadFileStream : public FileStream
{
public:
    ReadFileStream(const char* filename, std::size_t bufferSize=DefaultFileBufferSize);
    ReadFileStream(std::FILE* fp, std::size_t bufferSize=DefaultFileBufferSize);
    ReadFileStream(std::size_t bufferSize=DefaultFileBufferSize);

    virtual void SetFilePointer(std::FILE* fp) { fp_ = fp; Read(); }
    virtual bool Eof() const;
    virtual char Peek() const { return *current_; }
    virtual char Get();
    virtual std::size_t Read(char* ptr, std::size_t length);

    virtual std::size_t Tell() const { return count_; }

private:
    void Read();

    bool eof_;
    char* bufferLast_;
    char* current_;
    std::size_t count_;
};

////////////////////////////////////////////////////////////////////////////////

//! ReadStringStream uses an in memory buffer as a source for reading data.
//! The source buffer must be null terminated so the assumption is that this
//! is a standard text string.

class ANYRPC_API ReadStringStream : public Stream
{
public:
    ReadStringStream(const char *src) : src_(src), head_(src) {}

    virtual bool Eof() const { return (*src_ == '\0'); }
    virtual char Peek() const { return *src_; }
    virtual char Get();
    virtual std::size_t Read(char* ptr, std::size_t length);
    virtual std::size_t Tell() const { return static_cast<std::size_t>(src_ - head_); }

private:
    const char* src_;
    const char* head_;
};

////////////////////////////////////////////////////////////////////////////////

//! InSituStringStream uses an in memory buffer as a source for reading data.
//! The buffer does not need to be null terminated.
//! The memory buffer can be modified during the read.
//! Strings and binary data can be referenced from the buffer so the assumption
//! is that the buffer will be valid until all references are terminated.

class ANYRPC_API InSituStringStream : public Stream
{
public:
    InSituStringStream(char *src, std::size_t length, bool insitu=true, bool copy=false) :
        src_(src), head_(src), dst_(0), begin_(0), insitu_(insitu)
    {
        eof_ = src + length;
        copy_ = copy | (!insitu);
    }

    virtual bool InSitu() const { return insitu_; }
    virtual bool UseStringCopy() const { return copy_; }
    //! Set whether to actually using in place processing
    void SetInSitu(bool insitu) { insitu_ = insitu; if (!insitu_) copy_ = true; }
    //! Set whether to actually allow referencing of strings from the original source string
    void SetStringCopy(bool copy) { copy_ = copy; if (!insitu_) copy_ = true; }

    virtual bool Eof() const { return (src_ >= eof_);}

    virtual char Peek() const;
    virtual char Get();
    virtual char GetClear();
    virtual std::size_t Read(char* ptr, std::size_t length);
    virtual std::size_t Skip(std::size_t length);
    virtual char* PutBegin();
    virtual void Put(char c);
    virtual void Put(const char *str) { Put(str,strlen(str));}
    virtual void Put(const std::string &str) { Put(str.c_str(),str.length()); }
    virtual void Put(const char *str, std::size_t n);
    virtual std::size_t PutEnd();
    virtual std::size_t Tell() const { return static_cast<std::size_t>(src_ - head_); }

private:
    char* src_;         //!< Current location for reading data
    char* head_;        //!< Start of the buffer
    char* dst_;         //!< Location to write for Put calls
    char* begin_;       //!< Current location from the PutBegin
    char* eof_;         //!< End of the buffer
    bool insitu_;       //!< Process string in place
    bool copy_;         //!< Copy string to value
};

////////////////////////////////////////////////////////////////////////////////

//! WriteBufferedStream is the base class for streams that buffer all of the data in memory.
//! The additional calls are used to access the data after the data has been written to the stream.

class ANYRPC_API WriteBufferedStream : public Stream
{
public:
    //! Get a segment of data from the buffer.

    //! The offset indicates the position from the start of the buffer.
    //! The segmentLength is set for the length of this segment of data.
    //! The return pointer is the start of the segment of data.
    virtual const char* GetBuffer(std::size_t offset, std::size_t& segmentLength) = 0;
    //! Get the length of the buffered data.
    virtual std::size_t Length() = 0;
    //! Clear the buffered data and free any storage.
    virtual void Clear() = 0;
};

////////////////////////////////////////////////////////////////////////////////

//! WriteStringStream uses a std::string as the data buffer.
//! All of the data can be returned in a single call to GetBuffer.

class ANYRPC_API WriteStringStream : public WriteBufferedStream
{
public:
    WriteStringStream() {}
    WriteStringStream(std::size_t reserveSize) { str_.reserve(reserveSize); }

    virtual void Put(char c) { str_.push_back(c); }
    virtual void Put(const char *str) { str_.append(str); }
    virtual void Put(const std::string &str) { str_.append(str); }
    virtual void Put(const char *str, std::size_t n) { str_.append(str, n); }

    virtual const char* GetBuffer(std::size_t offset, std::size_t& segmentLength)
    {
        if (offset > str_.length())
        {
            segmentLength = 0;
            return 0;
        }
        segmentLength = str_.length()-offset;
        return str_.c_str() + offset;
    }
    virtual std::size_t Length() { return str_.length(); }
    virtual void Clear() { str_ = ""; }

    const char* GetBuffer() { return str_.c_str(); }
    const std::string& GetString() { return str_; }

private:
    std::string str_;
};

////////////////////////////////////////////////////////////////////////////////

//! WriteSegmentedStream builds a list of buffers to store the data.
//! The first buffer is preallocated with a static size.
//! Additional buffers are allocated with increasing sizes up to a maximum.
//!
//! Each buffer segment indicates whether it has been allocated so the Clear
//! function knows whether to free it.  This could also support referenced
//! data blocks with some modification.

class ANYRPC_API WriteSegmentedStream : public WriteBufferedStream
{
public:
    WriteSegmentedStream(size_t maxBufferSize=MaxBufferSize);
    virtual ~WriteSegmentedStream();

    virtual void Put(char c);
    virtual void Put(const char *str) { Put(str,strlen(str));}
    virtual void Put(const std::string &str) { Put(str.c_str(),str.length()); }
    virtual void Put(const char *str, std::size_t n);

    virtual const char* GetBuffer(std::size_t offset, std::size_t& segmentLength);
    virtual std::size_t Length() { return length_; }
    virtual void Clear();

private:
    void AddBuffer();

    struct BufferSegment
    {
        BufferSegment(char* buffer, std::size_t capacity, bool allocated=true) :
            buffer_(buffer), capacity_(capacity-1), used_(0), allocated_(allocated) {}

        char* buffer_;              //!< Pointer to the buffer
        std::size_t capacity_;      //!< Size of the buffer
        std::size_t used_;          //!< Current amount of the buffer being used
        bool allocated_;            //!< Whether the data was allocated (and should be freed)
    };
    typedef std::vector<BufferSegment> BufferList;

    BufferList buffers_;            //!< List of buffers for data
    std::size_t length_;            //!< Total length of the data in all buffers
    std::size_t nextCapacity_;      //!< Size of the next buffer to be allocated
    std::size_t maxBufferSize_;     //!< Maximum size for a buffer

    static const std::size_t StaticBufferSize = 1024;   //!< Size for the first static buffer
    static const std::size_t MaxBufferSize = 64*1024;   //!< Maximum size for an allocated buffer
    char staticBuffer[StaticBufferSize];                //!< First buffer
};

} // namespace anyrpc

////////////////////////////////////////////////////////////////////////////////

ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, char c);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, const char* str);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, const std::string& str);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, int i);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, unsigned u);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, int64_t i64);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, uint64_t u64);
ANYRPC_API anyrpc::Stream& operator<<(anyrpc::Stream& os, double d);

#endif // ANYRPC_STREAM_H_
