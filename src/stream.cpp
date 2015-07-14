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
#include "anyrpc/stream.h"

namespace anyrpc
{

FileStream::FileStream(size_t bufferSize) : bufferSize_(bufferSize)
{
    buffer_ = new char[bufferSize+1];
    fp_ = 0;
}

std::FILE* FileStream::GetFilePointer(bool resetFilePointer)
{
    std::FILE* fp = fp_;
    if (resetFilePointer)
        fp_ = 0;
    return fp;
}

void FileStream::Close()
{
    if (fp_ != 0)
    {
        Flush();
        fclose(fp_);
        fp_ = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

WriteFileStream::WriteFileStream(const char* filename, size_t bufferSize) :
    FileStream(bufferSize)
{
    fp_ = fopen(filename, "w");
    current_ = buffer_;
    bufferLast_ = buffer_ + bufferSize_;
}

WriteFileStream::WriteFileStream(std::FILE* fp, size_t bufferSize) :
    FileStream(bufferSize)
{
    fp_ = fp;
    current_ = buffer_;
    bufferLast_ = buffer_ + bufferSize_;
}

WriteFileStream::WriteFileStream(size_t bufferSize) :
    FileStream(bufferSize)
{
    fp_ = 0;
    current_ = buffer_;
    bufferLast_ = buffer_ + bufferSize_;
}

void WriteFileStream::Put(char c)
{
    *current_++ = c;
    if (current_ == bufferLast_)
        Flush();
}

void WriteFileStream::Put(const char *str, size_t n)
{
    size_t availableSpace = bufferLast_ - current_;
    if (availableSpace <= n)
    {
        // flush the current data and then write the new data directly to the file
        Flush();
        fwrite(str, 1, n, fp_);
    }
    else
    {
        // add the data to the buffer
        memcpy(current_, str, n);
        current_ += n;
    }
}

void WriteFileStream::Flush()
{
    if (current_ > buffer_)
    {
        fwrite(buffer_, 1, (current_ - buffer_), fp_);
        fflush(fp_);
        current_ = buffer_;
    }
}

////////////////////////////////////////////////////////////////////////////////

ReadFileStream::ReadFileStream(const char* filename, size_t bufferSize):
    FileStream(bufferSize)
{
    fp_ = fopen(filename, "r");
    eof_ = false;
    current_ = buffer_;
    bufferLast_ = buffer_;
    count_ = 0;
    Read();
}

ReadFileStream::ReadFileStream(std::FILE* fp, size_t bufferSize):
    FileStream(bufferSize)
{
    fp_ = fp;
    eof_ = false;
    current_ = buffer_;
    bufferLast_ = buffer_;
    count_ = 0;
    Read();
}

ReadFileStream::ReadFileStream(size_t bufferSize):
    FileStream(bufferSize)
{
    fp_ = 0;
    eof_ = false;
    current_ = buffer_;
    bufferLast_ = buffer_;
    count_ = 0;
}

bool ReadFileStream::Eof() const
{
    // Only end of file if read all of the data from the file and
    // all of the data from the internal buffer
    return eof_ && (current_ == bufferLast_);
}

char ReadFileStream::Get()
{
    char c = *current_;
    if (!Eof())
    {
        current_++;
        count_++;
        Read();
    }
    return c;
}

size_t ReadFileStream::Read(char* ptr, size_t length)
{
    size_t bytesRead = 0;
    while ((length > 0) && !Eof())
    {
        // copy up to the available data in the buffer
        size_t availBytes = std::min(static_cast<size_t>(bufferLast_ - current_), length);
        memcpy(ptr, current_, availBytes);
        length -= availBytes;
        ptr += availBytes;
        current_ += availBytes;
        bytesRead += availBytes;
        // get more data
        Read();
    }
    count_ += bytesRead;
    return bytesRead;
}

void ReadFileStream::Read()
{
    if (!eof_ && (current_ == bufferLast_) && (fp_ != 0))
    {
        // read more data from the file into the buffer
        size_t bytesRead = fread(buffer_, 1, bufferSize_, fp_);
        current_ = buffer_;
        bufferLast_ = buffer_ + bytesRead;
        if (bytesRead < bufferSize_)
        {
            eof_ = true;
            *bufferLast_ = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

char ReadStringStream::Get()
{
    if (*src_ == '\0')
        return *src_;
    return *src_++;
}

size_t ReadStringStream::Read(char* ptr, size_t length)
{
    // since the string is null terminated, have to read one character at a time
    size_t count;
    for (count=0; count<length; count++)
    {
        if (*src_ == '\0')
            break;
        *ptr++ = *src_++;
    }
    return count;
}

////////////////////////////////////////////////////////////////////////////////

char InSituStringStream::Peek() const
{
    if (Eof())
        return 0;
    return *src_;
}

char InSituStringStream::Get()
{
    if (Eof())
        return 0;
    return *src_++;
}

char InSituStringStream::GetClear()
{
    // get the character and then clear value
    if (Eof())
        return 0;
    char value = *src_;
    *src_++ = 0;
    return value;
}

size_t InSituStringStream::Read(char* ptr, size_t length)
{
    length = std::min(static_cast<size_t>(eof_ - src_), length);
    memcpy(ptr,src_,length);
    src_ += length;
    return length;
}

size_t InSituStringStream::Skip(size_t length)
{
    anyrpc_assert(dst_ != 0, AnyRpcErrorIllegalCall, "PutBegin was not called");

    if (dst_ != 0)
    {
        length = std::min(static_cast<size_t>(eof_ - src_), length);
        src_ += length;
        dst_ += length;
    }
    else
        length = 0;

    return length;
}

char* InSituStringStream::PutBegin()
{
    dst_ = src_;
    begin_ = src_;
    return begin_;
}

void InSituStringStream::Put(char c)
{
    anyrpc_assert(dst_ != 0, AnyRpcErrorIllegalCall, "PutBegin was not called");
    if (dst_ != 0)
    {
        *dst_ = c;
        if (dst_ < src_)
            dst_++;
        else
            anyrpc_assert(false, AnyRpcErrorBufferOverrun, "Buffer overrun, src_=" << src_);
    }
}

void InSituStringStream::Put(const char *str, size_t n)
{
    anyrpc_assert(dst_ != 0, AnyRpcErrorIllegalCall, "PutBegin was not called");
    if (dst_ != 0)
    {
        anyrpc_assert( src_ >=  (dst_ + n), AnyRpcErrorBufferOverrun, "Dst will overrun src: dst_=" << (void*)dst_ << ", src_=" << (void*)src_ << ", len=" << n );
        n = std::min(static_cast<size_t>(src_ - dst_), n);
        memcpy(dst_,str,n);
        dst_ += n;
    }
}

size_t InSituStringStream::PutEnd()
{
    size_t size = dst_ - begin_;
    dst_ = 0;
    begin_ = 0;
    return size;
}

////////////////////////////////////////////////////////////////////////////////

WriteSegmentedStream::WriteSegmentedStream(size_t maxBufferSize) : length_(0)
{
    buffers_.push_back( BufferSegment(staticBuffer,StaticBufferSize,false));
    maxBufferSize_ = maxBufferSize;
    nextCapacity_ = std::min( 2*StaticBufferSize, maxBufferSize_ );
}

WriteSegmentedStream::~WriteSegmentedStream()
{
    // need to free all of the allocated buffers
    for (BufferList::iterator it = buffers_.begin(); it != buffers_.end(); ++it)
        if (it->allocated_)
            free(it->buffer_);
}

void WriteSegmentedStream::Clear()
{
    for (BufferList::iterator it = buffers_.begin(); it != buffers_.end(); ++it)
        if (it->allocated_)
            free(it->buffer_);

    buffers_.clear();
    buffers_.push_back( BufferSegment(staticBuffer,StaticBufferSize,false) );
    nextCapacity_ = std::min( 2*StaticBufferSize, maxBufferSize_ );
    length_ = 0;
}

void WriteSegmentedStream::Put(char c)
{
    // check if enough room in the current allocation
    if (buffers_.back().capacity_ == buffers_.back().used_)
    {
        // allocate more space
        AddBuffer();
    }

    // add the character
    BufferSegment& backBuffer = buffers_.back();
    backBuffer.buffer_[backBuffer.used_] = c;
    backBuffer.used_++;
    length_++;
}

void WriteSegmentedStream::Put(const char *str, size_t n)
{
    while (n > 0)
    {
        // check if enough room in the current allocation
        BufferSegment& backBuffer = buffers_.back();
        size_t availableSpace = backBuffer.capacity_ - backBuffer.used_;
        if (availableSpace >= n)
        {
            // all of the remaining data will fit in the buffer
            memcpy(backBuffer.buffer_ + backBuffer.used_, str, n);
            backBuffer.used_ += n;
            length_ += n;
            break;
        }
        if (availableSpace > 0)
        {
            // add what you can to the current buffer
            memcpy(backBuffer.buffer_ + backBuffer.used_, str, availableSpace);
            backBuffer.used_ += availableSpace;
            length_ += availableSpace;
            str += availableSpace;
            n -= availableSpace;
        }
        // allocate more space
        AddBuffer();
    }
}

const char* WriteSegmentedStream::GetBuffer(size_t offset, size_t& segmentLength)
{
    segmentLength = 0;

    // Check if the request is within the available data
    if (offset >= length_)
        return 0;

    // search through the buffers until to find the right segment
    for (BufferList::iterator it = buffers_.begin(); it != buffers_.end(); ++it)
    {
        if (offset < it->used_)
        {
            // the offset starts inside this segment
            segmentLength = it->used_ - offset;
            // note that the capacity is one less than the allocated length to allow the null termination
            it->buffer_[it->used_] = 0;
            log_debug("GetBuffer: segmentLength=" << segmentLength);
            return it->buffer_ + offset;
        }
        offset -= it->used_;
    }
    log_warn("Failure finding buffer position");
    return 0;
}

void WriteSegmentedStream::AddBuffer()
{
    // allocate new buffer and put at the end of the list
    buffers_.push_back(BufferSegment(static_cast<char*>(malloc(nextCapacity_)), nextCapacity_));
    // double the capacity for the next buffer
    nextCapacity_ = std::min( 2*nextCapacity_, maxBufferSize_ );
}

} // namespace anyrpc

////////////////////////////////////////////////////////////////////////////////

anyrpc::Stream& operator<<(anyrpc::Stream& os, char c)
{
    os.Put(c);
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, const char* str)
{
    os.Put(str);
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, const std::string& str)
{
    os.Put(str);
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, int i)
{
    std::stringstream buffer;
    buffer << i;
    os.Put(buffer.str());
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, unsigned u)
{
    std::stringstream buffer;
    buffer << u;
    os.Put(buffer.str());
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, int64_t i64)
{
    std::stringstream buffer;
    buffer << i64;
    os.Put(buffer.str());
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, uint64_t u64)
{
    std::stringstream buffer;
    buffer << u64;
    os.Put(buffer.str());
    return os;
}

anyrpc::Stream& operator<<(anyrpc::Stream& os, double d)
{
    char buffer[25];
    int length = snprintf(buffer, sizeof(buffer), "%g", d);
    os.Put( buffer, length );
    return os;
}
