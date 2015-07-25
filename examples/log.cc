#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "IOBuffer.hpp"
#include "Binlog.hpp"

int main(int argc, char* argv[])
{
    printf("sizeof(BinlogHead)      : %lu\n", sizeof(BinlogHead));
    printf("sizeof(BinlogBlockHead) : %lu\n", sizeof(BinlogBlockHead));

    Binlog stlog;
    if(-1 == stlog.CreateLog("test.binlog", BINLOG_1MB))
    {
        printf("open binlog fail.\n");
        return -1;
    }

    uint32_t i = 0;
    for(; i < 5; ++i)
    {
        off_t offset = stlog.Write((char*)&i, sizeof(uint32_t));
        printf("%d: 0x%lx\n", i, offset);
    }

    Binlog::IteratorType iter = stlog.GetIterator(0x101c);
    //Binlog::IteratorType iter = stlog.GetIterator();
    while(stlog.Next(&iter) > 0)
    {
        uint32_t* pData = (uint32_t*)iter.GetBuffer();
        printf("0x%lx: %u\n", iter.GetOffset(), *pData);

        if(i < 10)
        {
            stlog.Write((char*)&i, sizeof(uint32_t));
            ++i;
        }
    }
    iter.Release();

    stlog.Close();
    return 0;
}


