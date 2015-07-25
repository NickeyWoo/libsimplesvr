/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2015-04-23
 *
--*/
#ifndef __BINLOG_HPP__
#define __BINLOG_HPP__

#include <boost/static_assert.hpp>
#include "IOBuffer.hpp"

#define BINLOG_1MB              1048576
#define BINLOG_2MB              2097152
#define BINLOG_4MB              4194304
#define BINLOG_8MB              8388608
#define BINLOG_16MB             16777216
#define BINLOG_32MB             33554432
#define BINLOG_64MB             67108864
#define BINLOG_128MB            134217728
#define BINLOG_256MB            268435456
#define BINLOG_512MB            536870912

#define BINLOG_SMAGIC           (uint8_t)0x03
#define BINLOG_EMAGIC           (uint8_t)0x0a

#define BINLOG_HEAD_MAGIC       "#BINLOG#"
#define BINLOG_HEAD_VERSION     0x0101
#define BINLOG_HEAD_SIZE        4096

#define BINLOG_BLOCKHEAD_MAGIC  (uint8_t)0xEF
#define BINLOG_BLOCKHEAD_SIZE   8

struct BinlogBlockHead
{
    uint8_t  cMagic;
    uint32_t dwCreateTime;
};

struct BinlogHead 
{
    char     cMagic[8];
    uint16_t wVersion;
    uint16_t wDataOffset;
    uint32_t dwBlockSize;
    uint32_t dwCreateTime;
};

IOBuffer& operator >> (IOBuffer& in, BinlogHead& stHead);
IOBuffer& operator << (IOBuffer& out, BinlogHead& stHead);

IOBuffer& operator >> (IOBuffer& in, BinlogBlockHead& stBlockHead);
IOBuffer& operator << (IOBuffer& out, BinlogBlockHead& stBlockHead);

struct BinlogIterator
{
    uint32_t dwBlockIndex;
    uint32_t dwOffset;

    uint32_t dwBlockSize;

    char* cBlockBuffer;
    int dwRealSize;

    BinlogBlockHead stBlockHead;

    char* cLogBuffer;
    uint32_t dwLogSize;
    off_t dwLogOffset;

    BinlogIterator();
    void Release();

    inline off_t GetOffset()
    {
        return dwLogOffset;
    }

    inline const char* GetBuffer()
    {
        return cLogBuffer;
    }

    inline size_t GetSize()
    {
        return dwLogSize;
    }
};

class Binlog
{
public:
    typedef BinlogIterator IteratorType;

    Binlog();

    int OpenLog(const char* szPath);
    int CreateLog(const char* szPath, uint32_t dwBlockSize = BINLOG_16MB);
    void Close();

    off_t Write(const char* buffer, size_t size);
    
    size_t GetSize();

    IteratorType GetIterator();
    IteratorType GetIterator(off_t offset);
    int Next(IteratorType* pIter);

    inline bool IsOpen()
    {
        return (m_fdBinlog != -1);
    }

    inline BinlogHead* GetFileInfo()
    {
        return &m_stHead;
    }

private:

    // log = SMAGIC + dwDataLen + cData + EMAGIC
    inline size_t GetLogSize(size_t size)
    {
        return sizeof(BINLOG_SMAGIC) + sizeof(uint32_t) + size + sizeof(BINLOG_EMAGIC);
    }

    int         m_fdBinlog;
    BinlogHead  m_stHead;

};

#endif // __BINLOG_HPP__
