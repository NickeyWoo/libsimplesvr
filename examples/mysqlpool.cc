#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/format.hpp>
#include <nameapi.h>

extern "C" {

#include <mysql.h>
#include <hiredis.h>

}

#include "LoadBalance.hpp"
#include "ConnectionPool.hpp"

int MySQLFunc(const char* szSQL)
{
    sockaddr_in stAddr;
    bzero(&stAddr, sizeof(sockaddr_in));

    stAddr.sin_family = PF_INET;
    stAddr.sin_port = htons(3613);
    stAddr.sin_addr.s_addr = inet_addr("10.189.30.38");

    MYSQL* pMySQL = NULL;
    ConnectionPool<MYSQL>& stPool = PoolObject<ConnectionPool<MYSQL> >::Instance();
    stPool.Attach(stAddr, &pMySQL);
    if(!pMySQL->host || mysql_ping(pMySQL) != 0)
    {
        if(NULL == mysql_real_connect(pMySQL, "10.189.30.38",
                                      "appnews_mq", "8b07ad0b5", 
                                      "appnews_mq", 3613, NULL, 0))
        {
            printf("error: %s\n", mysql_error(pMySQL));

            stPool.Detach(pMySQL);
            return -1;
        }
    }

    if(mysql_query(pMySQL, szSQL) != 0)
    {
        stPool.Detach(pMySQL);
        return 0;
    }

    MYSQL_RES* pResult = mysql_store_result(pMySQL);
    MYSQL_ROW row;
    while((row = mysql_fetch_row(pResult)))
    {
        printf("%s\n", row[0]);
    }
    mysql_free_result(pResult);

    stPool.Detach(pMySQL);
    return 0;
}

int RedisFunc(const char* szCmd)
{
    sockaddr_in stAddr;
    bzero(&stAddr, sizeof(sockaddr_in));

    stAddr.sin_family = PF_INET;
    stAddr.sin_port = htons(9004);
    stAddr.sin_addr.s_addr = inet_addr("10.240.119.131");

    REDIS* pRedis = NULL;
    ConnectionPool<REDIS>& stPool = PoolObject<ConnectionPool<REDIS> >::Instance();
    stPool.Attach(stAddr, &pRedis);

    if(pRedis->GetErrno() != 0)
    {
        timeval tv = {0, 200000};
        if(0 != pRedis->ConnectWithTimeout(stAddr, tv))
        {
            printf("error(%d): %s\n", pRedis->GetErrno(), pRedis->GetError());
            stPool.Detach(pRedis);
            return -1;
        }
    }

    redisReply* pReply = (redisReply*)redisCommand(*pRedis, szCmd);
    if(pReply == NULL)
    {
        stPool.Detach(pRedis);
        return -1;
    }
    switch(pReply->type)
    {
    case REDIS_REPLY_STRING:
        printf("%s\n", pReply->str);
        break;
    case REDIS_REPLY_INTEGER:
        printf("%llu\n", pReply->integer);
        break;
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_ERROR:
        printf("%s\n", pReply->str);
        break;
    }
    freeReplyObject(pReply);

    stPool.Detach(pRedis);
    return 0;
}

int main(int argc, char* argv[])
{
    MySQLFunc("SHOW TABLES");
    MySQLFunc("SHOW DATABASES");

    printf("\n////////////////////////// REDIS //////////////////////////\n");

    RedisFunc("SET TEST 1");
    RedisFunc("INCR TEST");
    RedisFunc("DEL TEST");

    return 0;
}


