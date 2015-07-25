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
#include <sys/time.h>
#include <time.h>
#include <string>
#include <boost/format.hpp>
#include "Binlog.hpp"

IOBuffer& operator >> (IOBuffer& in, BinlogHead& stHead)
{
    in.Read(stHead.cMagic, 8);
    in >> stHead.wVersion >> stHead.wDataOffset 
       >> stHead.dwBlockSize >> stHead.dwCreateTime;
    return in;
}

IOBuffer& operator << (IOBuffer& out, BinlogHead& stHead)
{
    out.Write(stHead.cMagic, 8);
    out << stHead.wVersion << stHead.wDataOffset 
        << stHead.dwBlockSize << stHead.dwCreateTime;
    return out;
}

IOBuffer& operator >> (IOBuffer& in, BinlogBlockHead& stBlockHead)
{
    in >> stBlockHead.cMagic >> stBlockHead.dwCreateTime;
    return in;
}

IOBuffer& operator << (IOBuffer& out, BinlogBlockHead& stBlockHead)
{
    out << stBlockHead.cMagic << stBlockHead.dwCreateTime;
    return out;
}

Binlog::Binlog() :
    m_fdBinlog(-1)
{
}

int Binlog::OpenLog(const char* szPath)
{
    if(m_fdBinlog != -1) close(m_fdBinlog);

    m_fdBinlog = open(szPath, O_APPEND|O_RDWR, 0666);
    if(m_fdBinlog == -1)
        return -1;

    char buffer[BINLOG_HEAD_SIZE];
    if(-1 == pread(m_fdBinlog, buffer, BINLOG_HEAD_SIZE, 0))
    {
        close(m_fdBinlog);
        return -1;
    }

    IOBuffer in(buffer, BINLOG_HEAD_SIZE, BINLOG_HEAD_SIZE);
    in >> m_stHead;

    if(memcmp(m_stHead.cMagic, BINLOG_HEAD_MAGIC, 8) != 0 ||
        m_stHead.wVersion != BINLOG_HEAD_VERSION)
    {
        close(m_fdBinlog);
        return -1;
    }

    return 0;
}

int Binlog::CreateLog(const char* szPath, uint32_t dwBlockSize)
{
    if(m_fdBinlog != -1) close(m_fdBinlog);

    m_fdBinlog = open(szPath, O_CREAT|O_RDWR, 0666);
    if(m_fdBinlog == -1)
        return -1;

    ftruncate(m_fdBinlog, 0);

    bzero(&m_stHead, sizeof(BinlogHead));

    strncpy(m_stHead.cMagic, BINLOG_HEAD_MAGIC, 8);
    m_stHead.wVersion = BINLOG_HEAD_VERSION;
    m_stHead.wDataOffset = BINLOG_HEAD_SIZE;
    m_stHead.dwBlockSize = dwBlockSize;
    m_stHead.dwCreateTime = (uint32_t)time(NULL);

    char cFileHeadBuf[BINLOG_HEAD_SIZE];
    bzero(cFileHeadBuf, BINLOG_HEAD_SIZE);

    IOBuffer ioFileHead(cFileHeadBuf, BINLOG_HEAD_SIZE);
    ioFileHead << m_stHead;

    if(-1 == write(m_fdBinlog, cFileHeadBuf, BINLOG_HEAD_SIZE))
        return -1;

    return 0;
}

void Binlog::Close()
{
    close(m_fdBinlog);
}

size_t Binlog::GetSize()
{
    struct stat stStat;
    bzero(&stStat, sizeof(struct stat));

    if(-1 == fstat(m_fdBinlog, &stStat))
        return 0;

    return stStat.st_size;
}

off_t Binlog::Write(const char* buffer, size_t size)
{
    if(m_fdBinlog == -1)
        return -1;

    uint32_t dwRealSize = GetLogSize(size);
    if(dwRealSize > m_stHead.dwBlockSize)
        return -1;

    struct stat stStat;
    if(-1 == fstat(m_fdBinlog, &stStat))
        return -1;

    uint32_t dwRemainSize = m_stHead.dwBlockSize - ((stStat.st_size - m_stHead.wDataOffset) % m_stHead.dwBlockSize);

    off_t offset = stStat.st_size;
    iovec stIOVec[5];
    bzero(&stIOVec, 5 * sizeof(iovec));
    int iVecSize = 0;

    char cBlockHeadBuf[BINLOG_BLOCKHEAD_SIZE];
    bzero(cBlockHeadBuf, BINLOG_BLOCKHEAD_SIZE);

    if(dwRemainSize == m_stHead.dwBlockSize)
    {
        // 输出块头
        BinlogBlockHead stBlockHead;
        bzero(&stBlockHead, sizeof(BinlogBlockHead));
        stBlockHead.cMagic = BINLOG_BLOCKHEAD_MAGIC;
        stBlockHead.dwCreateTime = (uint32_t)time(NULL);

        IOBuffer ioBlockBuf(cBlockHeadBuf, BINLOG_BLOCKHEAD_SIZE);
        ioBlockBuf << stBlockHead;

        stIOVec[iVecSize].iov_base = cBlockHeadBuf;
        stIOVec[iVecSize].iov_len = BINLOG_BLOCKHEAD_SIZE;
        ++iVecSize;

        offset += BINLOG_BLOCKHEAD_SIZE;
    }
    else if(dwRemainSize < dwRealSize)
    {
        // 补全, 输出块头
        if(-1 == lseek(m_fdBinlog, dwRemainSize, SEEK_CUR))
            return -1;

        BinlogBlockHead stBlockHead;
        bzero(&stBlockHead, sizeof(BinlogBlockHead));
        stBlockHead.cMagic = BINLOG_BLOCKHEAD_MAGIC;
        stBlockHead.dwCreateTime = (uint32_t)time(NULL);

        IOBuffer ioBlockBuf(cBlockHeadBuf, BINLOG_BLOCKHEAD_SIZE);
        ioBlockBuf << stBlockHead;

        stIOVec[iVecSize].iov_base = cBlockHeadBuf;
        stIOVec[iVecSize].iov_len = BINLOG_BLOCKHEAD_SIZE;
        ++iVecSize;

        offset += dwRemainSize + BINLOG_BLOCKHEAD_SIZE;
    }

    uint8_t cMagicS = BINLOG_SMAGIC, cMagicE = BINLOG_EMAGIC;
    uint32_t dwLogSize = htonl(size);

    stIOVec[iVecSize].iov_base = &cMagicS;
    stIOVec[iVecSize].iov_len = sizeof(cMagicS);
    ++iVecSize;

    stIOVec[iVecSize].iov_base = &dwLogSize;
    stIOVec[iVecSize].iov_len = sizeof(dwLogSize);
    ++iVecSize;

    stIOVec[iVecSize].iov_base = (void*)buffer;
    stIOVec[iVecSize].iov_len = size;
    ++iVecSize;

    stIOVec[iVecSize].iov_base = &cMagicE;
    stIOVec[iVecSize].iov_len = sizeof(cMagicE);
    ++iVecSize;

    if(-1 == writev(m_fdBinlog, stIOVec, iVecSize))
        return -1;

    return offset;
}

BinlogIterator::BinlogIterator() :
    dwBlockIndex(0),
    dwOffset(0),
    dwBlockSize(0),
    cBlockBuffer(NULL),
    dwRealSize(0),
    cLogBuffer(NULL),
    dwLogSize(0)
{
}

void BinlogIterator::Release()
{
    if(cBlockBuffer)
    {
        free(cBlockBuffer);

        dwBlockIndex = 0;
        dwOffset = 0;

        dwBlockSize = 0;
        cBlockBuffer = NULL;
        dwRealSize = 0;

        cLogBuffer = NULL;
        dwLogSize = 0;
    }
}

Binlog::IteratorType Binlog::GetIterator()
{
    BinlogIterator iter;
    iter.dwBlockIndex = 0;
    iter.dwOffset = 0;
    iter.cBlockBuffer = (char*)malloc(m_stHead.dwBlockSize);
    iter.dwBlockSize = m_stHead.dwBlockSize;
    return iter;
}

Binlog::IteratorType Binlog::GetIterator(off_t offset)
{
    if(offset < m_stHead.wDataOffset)
        return GetIterator();

    BinlogIterator iter;
    iter.dwBlockIndex = (offset - m_stHead.wDataOffset) / m_stHead.dwBlockSize;
    iter.dwOffset = (offset - m_stHead.wDataOffset) % m_stHead.dwBlockSize;
    iter.cBlockBuffer = (char*)malloc(m_stHead.dwBlockSize);
    iter.dwBlockSize = m_stHead.dwBlockSize;
    return iter;
}

int Binlog::Next(IteratorType* pIter)
{
    if(pIter == NULL || pIter->dwBlockSize < m_stHead.dwBlockSize)
        return -1;

    do
    {
        while(pIter->dwOffset >= (uint32_t)pIter->dwRealSize)
        {
            if(pIter->dwOffset >= m_stHead.dwBlockSize)
            {
                // read next block
                ++pIter->dwBlockIndex;

                off_t offset = m_stHead.wDataOffset + m_stHead.dwBlockSize * pIter->dwBlockIndex;
                pIter->dwRealSize = pread(m_fdBinlog, pIter->cBlockBuffer, m_stHead.dwBlockSize, offset);
                if(-1 == pIter->dwRealSize)
                    return -1;
                else if(pIter->dwRealSize == 0)
                    return 0;   // end of file
                else if(pIter->dwRealSize < BINLOG_BLOCKHEAD_SIZE)
                    return -1; // must more than BINLOG_BLOCKHEAD_SIZE

                IOBuffer ioBlockBuf(pIter->cBlockBuffer, pIter->dwRealSize, pIter->dwRealSize);
                ioBlockBuf >> pIter->stBlockHead; 

                if(pIter->stBlockHead.cMagic != BINLOG_BLOCKHEAD_MAGIC)
                {
                    // unknown block head, read next block
                    pIter->dwOffset = m_stHead.dwBlockSize;
                    continue;
                }

                pIter->dwOffset = BINLOG_BLOCKHEAD_SIZE;
            }
            else if(pIter->dwRealSize > 0 && (uint32_t)pIter->dwRealSize < m_stHead.dwBlockSize)
            {
                // block cache is not full, reload block cache
                size_t dwReadOffset = m_stHead.wDataOffset + m_stHead.dwBlockSize * pIter->dwBlockIndex + pIter->dwOffset;

                struct stat stStat;
                bzero(&stStat, sizeof(struct stat));
                if(fstat(m_fdBinlog, &stStat) == -1)
                    return -1;

                if((uint32_t)dwReadOffset >= stStat.st_size)
                    return 0; // end of file

                off_t offset = m_stHead.wDataOffset + m_stHead.dwBlockSize * pIter->dwBlockIndex;
                pIter->dwRealSize = pread(m_fdBinlog, pIter->cBlockBuffer, m_stHead.dwBlockSize, offset);
                if(-1 == pIter->dwRealSize)
                    return -1;
                else if(pIter->dwRealSize == 0)
                    return 0;   // end of file
                else if(pIter->dwRealSize < BINLOG_BLOCKHEAD_SIZE)
                    return -1; // must more than BINLOG_BLOCKHEAD_SIZE
            }
            else
            {
                // load block cache
                off_t offset = m_stHead.wDataOffset + m_stHead.dwBlockSize * pIter->dwBlockIndex;
                pIter->dwRealSize = pread(m_fdBinlog, pIter->cBlockBuffer, m_stHead.dwBlockSize, offset);
                if(-1 == pIter->dwRealSize)
                    return -1;
                else if(pIter->dwRealSize == 0)
                    return 0;   // end of file
                else if(pIter->dwRealSize < BINLOG_BLOCKHEAD_SIZE)
                    return -1; // must more than BINLOG_BLOCKHEAD_SIZE

                IOBuffer ioBlockBuf(pIter->cBlockBuffer, pIter->dwRealSize, pIter->dwRealSize);
                ioBlockBuf >> pIter->stBlockHead; 

                if(pIter->stBlockHead.cMagic != BINLOG_BLOCKHEAD_MAGIC)
                {
                    // unknown block head, read next block
                    pIter->dwOffset = m_stHead.dwBlockSize;
                    continue;
                }

                if(pIter->dwOffset < BINLOG_BLOCKHEAD_SIZE)
                    pIter->dwOffset = BINLOG_BLOCKHEAD_SIZE;
            }
        }

        off_t dwLogOffset = m_stHead.wDataOffset + m_stHead.dwBlockSize * pIter->dwBlockIndex + pIter->dwOffset;

        if(pIter->cBlockBuffer[pIter->dwOffset] != BINLOG_SMAGIC)
        {
            pIter->dwOffset = m_stHead.dwBlockSize;
            continue;
        }

        uint32_t dwLogBuffSize = pIter->dwRealSize - pIter->dwOffset;
        if(dwLogBuffSize < 6)
        {
            // binlog = STX + dwLen + cData + ETX
            // length of binlog must more than 6
            pIter->dwOffset = m_stHead.dwBlockSize;
            continue;
        }

        IOBuffer ioLogBuf(&pIter->cBlockBuffer[pIter->dwOffset], dwLogBuffSize, dwLogBuffSize);

        uint8_t cMagicS, cMagicE;
        uint32_t dwLogSize;
        ioLogBuf >> cMagicS >> dwLogSize;

        uint32_t dwLogRealSize = 6 + dwLogSize;
        if(dwLogBuffSize < dwLogRealSize)
        {
            pIter->dwOffset = m_stHead.dwBlockSize;
            continue;
        }

        pIter->dwLogSize = dwLogSize;
        pIter->cLogBuffer = ioLogBuf.GetReadBuffer();
        pIter->dwLogOffset = dwLogOffset;

        ioLogBuf.ReadSeek(dwLogSize);
        ioLogBuf >> cMagicE;

        if(cMagicE != BINLOG_EMAGIC)
        {
            pIter->dwOffset = m_stHead.dwBlockSize;
            continue;
        }

        pIter->dwOffset += dwLogRealSize;
        break;
    } while(true);

    return pIter->dwLogSize + 6;
}





