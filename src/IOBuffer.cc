/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "IOBuffer.hpp"

IOException::IOException(const char* errorMessage) :
    m_ErrorMessage(errorMessage)
{
}

IOException::~IOException() throw()
{
}

const char* IOException::what() const throw()
{
    return m_ErrorMessage.c_str();
}

OverflowIOException::OverflowIOException(const char* errorMessage) :
    IOException(errorMessage)
{
}

OverflowIOException::~OverflowIOException() throw()
{
}

const char* OverflowIOException::what() const throw()
{
    return this->m_ErrorMessage.c_str();
}

///////////////////////////////////////////////////////
IOBuffer::IOBuffer(size_t size) :
    m_NeedFree(true),
    m_BufferSize(size),
    m_ReadPosition(0),
    m_AvailableReadSize(0),
    m_WritePosition(0)
{
    m_Buffer = (char*)malloc(size);
}

IOBuffer::IOBuffer(char* buffer, size_t size) :
    m_NeedFree(false),
    m_Buffer(buffer),
    m_BufferSize(size),
    m_ReadPosition(0),
    m_AvailableReadSize(0),
    m_WritePosition(0)
{
}

IOBuffer::IOBuffer(char* buffer, size_t size, size_t avaliableReadSize) :
    m_NeedFree(false),
    m_Buffer(buffer),
    m_BufferSize(size),
    m_ReadPosition(0),
    m_AvailableReadSize(avaliableReadSize),
    m_WritePosition(0)
{
}

IOBuffer::~IOBuffer()
{
    if(m_NeedFree)
        free(m_Buffer);
}

ssize_t IOBuffer::Write(const char* buffer, size_t size)
{
    if(m_WritePosition + size > m_BufferSize)
        return 0;

    memcpy(m_Buffer + m_WritePosition, buffer, size);
    m_WritePosition += size;
    return size;
}

ssize_t IOBuffer::Write(const char* buffer, size_t size, size_t pos)
{
    if(pos + size > m_BufferSize)
        return 0;

    memcpy(m_Buffer + pos, buffer, size);
    return size;
}

ssize_t IOBuffer::Read(char* buffer, size_t size)
{
    if(m_ReadPosition + size > m_AvailableReadSize)
        return 0;

    memcpy(buffer, m_Buffer + m_ReadPosition, size);
    m_ReadPosition += size;
    return size;
}

ssize_t IOBuffer::Read(char* buffer, size_t size, size_t pos) const
{
    if(pos + size > m_AvailableReadSize)
        return 0;

    memcpy(buffer, m_Buffer + pos, size);
    return size;
}

void IOBuffer::Dump(std::string& str)
{
    size_t len = m_AvailableReadSize;
    if(m_AvailableReadSize < m_WritePosition)
        len = m_WritePosition;
    if(len == 0)
    {
        str.append("___|__0__1__2__3__4__5__6__7__8__9__A__B__C__D__E__F_|_________________\n");
        str.append("00 |                                                 |\n");
        return;
    }

    int iLineCount = len / 16;
    int iMod = len % 16;
    if(iMod != 0)
        ++iLineCount;

    int iLeftAlign = GetLeftAlignSize(iLineCount);
    std::string strLineNumFormat = (boost::format("%%0%dX0 | ") % iLeftAlign).str();

    str.append(iLeftAlign, '_');
    str.append("__|__0__1__2__3__4__5__6__7__8__9__A__B__C__D__E__F_|_________________\n");

    int iLineNum = 0;
    str.append((boost::format(strLineNumFormat) % iLineNum).str());

    char szStrBuffer[17] = {0x0};
    for(size_t i=0; i<len; ++i)
    {
        unsigned char c = (unsigned char)m_Buffer[i];
        str.append((boost::format("%02X ") % (uint32_t)c).str());

        int idx = i % 16;
        if(c > 31 && c < 127)
            szStrBuffer[idx] = c;
        else
            szStrBuffer[idx] = '.';

        if((i + 1) % 16 == 0)
        {
            str.append((boost::format("| %s\n") % szStrBuffer).str());
            if((i + 1) >= len)
                break;

            memset(szStrBuffer, 0, 17);
            ++iLineNum;
            str.append((boost::format(strLineNumFormat) % iLineNum).str());
        }
    }
    if(iMod)
    {
        str.append((16 - iMod) * 3, ' ');
        str.append((boost::format("| %s\n") % szStrBuffer).str());
    }
}

int IOBuffer::GetLeftAlignSize(long long int llNum)
{
    char buffer[23];
    memset(buffer, 0, 23);
    sprintf(buffer, "%llx", llNum);
    return strlen(buffer);
}

IOBuffer& operator >> (IOBuffer& io, char& val)
{
    if(io.m_ReadPosition + sizeof(char) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = io.m_Buffer[io.m_ReadPosition];
    ++io.m_ReadPosition;
    return io;
}

IOBuffer& operator >> (IOBuffer& io, unsigned char& val)
{
    if(io.m_ReadPosition + sizeof(unsigned char) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = (unsigned char)io.m_Buffer[io.m_ReadPosition];
    ++io.m_ReadPosition;
    return io;
}

IOBuffer& operator >> (IOBuffer& io, int16_t& val)
{
    if(io.m_ReadPosition + sizeof(int16_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = *((int16_t*)(io.m_Buffer + io.m_ReadPosition));
    val = ntohs(val);
    io.m_ReadPosition += sizeof(int16_t);
    return io;
}

IOBuffer& operator >> (IOBuffer& io, uint16_t& val)
{
    if(io.m_ReadPosition + sizeof(uint16_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = *((uint16_t*)(io.m_Buffer + io.m_ReadPosition));
    val = ntohs(val);
    io.m_ReadPosition += sizeof(uint16_t);
    return io;
}

IOBuffer& operator >> (IOBuffer& io, int32_t& val)
{
    if(io.m_ReadPosition + sizeof(int32_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = *((int32_t*)(io.m_Buffer + io.m_ReadPosition));
    val = ntohl(val);
    io.m_ReadPosition += sizeof(int32_t);
    return io;
}

IOBuffer& operator >> (IOBuffer& io, uint32_t& val)
{
    if(io.m_ReadPosition + sizeof(uint32_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = *((uint32_t*)(io.m_Buffer + io.m_ReadPosition));
    val = ntohl(val);
    io.m_ReadPosition += sizeof(uint32_t);
    return io;
}

IOBuffer& operator >> (IOBuffer& io, int64_t& val)
{
    if(io.m_ReadPosition + sizeof(int64_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = *((int64_t*)(io.m_Buffer + io.m_ReadPosition));
    val = ntohll(val);
    io.m_ReadPosition += sizeof(int64_t);
    return io;
}

IOBuffer& operator >> (IOBuffer& io, uint64_t& val)
{
    if(io.m_ReadPosition + sizeof(uint64_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    val = *((uint64_t*)(io.m_Buffer + io.m_ReadPosition));
    val = ntohll(val);
    io.m_ReadPosition += sizeof(uint64_t);
    return io;
}

IOBuffer& operator >> (IOBuffer& io, std::string& val)
{
    if(io.m_ReadPosition + sizeof(uint16_t) > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    uint16_t len = *((uint16_t*)(io.m_Buffer + io.m_ReadPosition));
    len = ntohs(len);

    if(io.m_ReadPosition + sizeof(uint16_t) + len > io.m_AvailableReadSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

    io.m_ReadPosition += sizeof(uint16_t);
    val = std::string(io.m_Buffer + io.m_ReadPosition, len);
    io.m_ReadPosition += len;
    return io;
}

IOBuffer& operator << (IOBuffer& io, char val)
{
    if(io.m_WritePosition + sizeof(char) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    io.m_Buffer[io.m_WritePosition] = val;
    ++io.m_WritePosition;
    return io;
}

IOBuffer& operator << (IOBuffer& io, unsigned char val)
{
    if(io.m_WritePosition + sizeof(char) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    io.m_Buffer[io.m_WritePosition] = (char)val;
    ++io.m_WritePosition;
    return io;
}

IOBuffer& operator << (IOBuffer& io, int16_t val)
{
    if(io.m_WritePosition + sizeof(int16_t) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    val = htons(val);
    *((int16_t*)(io.m_Buffer + io.m_WritePosition)) = val;
    io.m_WritePosition += sizeof(int16_t);
    return io;
}

IOBuffer& operator << (IOBuffer& io, uint16_t val)
{
    if(io.m_WritePosition + sizeof(uint16_t) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    val = htons(val);
    *((uint16_t*)(io.m_Buffer + io.m_WritePosition)) = val;
    io.m_WritePosition += sizeof(uint16_t);
    return io;
}

IOBuffer& operator << (IOBuffer& io, int32_t val)
{
    if(io.m_WritePosition + sizeof(int32_t) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    val = htonl(val);
    *((int32_t*)(io.m_Buffer + io.m_WritePosition)) = val;
    io.m_WritePosition += sizeof(int32_t);
    return io;
}

IOBuffer& operator << (IOBuffer& io, uint32_t val)
{
    if(io.m_WritePosition + sizeof(uint32_t) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    val = htonl(val);
    *((uint32_t*)(io.m_Buffer + io.m_WritePosition)) = val;
    io.m_WritePosition += sizeof(uint32_t);
    return io;
}

IOBuffer& operator << (IOBuffer& io, int64_t val)
{
    if(io.m_WritePosition + sizeof(int64_t) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    val = htonll(val);
    *((int64_t*)(io.m_Buffer + io.m_WritePosition)) = val;
    io.m_WritePosition += sizeof(int64_t);
    return io;
}

IOBuffer& operator << (IOBuffer& io, uint64_t val)
{
    if(io.m_WritePosition + sizeof(uint64_t) > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    val = htonll(val);
    *((uint64_t*)(io.m_Buffer + io.m_WritePosition)) = val;
    io.m_WritePosition += sizeof(uint64_t);
    return io;
}

IOBuffer& operator << (IOBuffer& io, std::string val)
{
    if(io.m_WritePosition + sizeof(uint16_t) + val.length() > io.m_BufferSize)
        throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

    uint16_t len = htons(val.length());
    *((uint16_t*)(io.m_Buffer + io.m_WritePosition)) = len;
    io.m_WritePosition += sizeof(uint16_t);
    memcpy(io.m_Buffer + io.m_WritePosition, val.c_str(), val.length());
    io.m_WritePosition += val.length();
    return io;
}







