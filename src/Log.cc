/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
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
#include <pthread.h>
#include "Log.hpp"
#include "PoolObject.hpp"
#include "Pool.hpp"
#include "Timer.hpp"

pthread_once_t safe_strerror_once = PTHREAD_ONCE_INIT;
pthread_key_t safe_strerror_key;

void safe_strerror_free(void* buffer)
{
    free(buffer);
}

void safe_strerror_init()
{
    pthread_key_create(&safe_strerror_key, &safe_strerror_free);
}

const char* safe_strerror(int error)
{
#if defined(POOL_USE_THREADPOOL)
    pthread_once(&safe_strerror_once, &safe_strerror_init);

    char* buffer = (char*)pthread_getspecific(safe_strerror_key);
    if(!buffer)
    {
        buffer = (char*)malloc(256);
        pthread_setspecific(safe_strerror_key, buffer);
    }

    if(error < 0 || error >= _sys_nerr || _sys_errlist[error] == NULL)
        snprintf(buffer, 256, "Unknown error %d", error);
    else
        snprintf(buffer, 256, "%s", _sys_errlist[error]);

    return buffer;
#else
    return strerror(error);
#endif
}

SimpleLog::SimpleLog() :
    m_File(NULL)
{
}

SimpleLog::~SimpleLog()
{
    Close();
}

bool SimpleLog::Initialize(std::string path)
{
    m_Path = path;
    if(m_Path.empty())
        m_Path = std::string(".");

    time_t now = time(NULL) / 3600 * 3600;
    tm now_tm;
    localtime_r(&now, &now_tm);

    char buffer[260];
    memset(buffer, 0, 260);
    snprintf(buffer, 259, "%s/log%04d%02d%02d%02d.log%u", 
                m_Path.c_str(),
                (now_tm.tm_year+1900),
                (now_tm.tm_mon+1),
                now_tm.tm_mday,
                now_tm.tm_hour,
                Pool::Instance().GetID());

    m_File = fopen(buffer, "a");
    if(m_File == NULL)
        return false;

    PoolObject<Timer<void> >::Instance().SetTimeout(boost::bind(&SimpleLog::Flush, this), LOG_FLUSH_TIMEOUT);
    return true;
}

void SimpleLog::Close()
{
    if(m_File)
    {
        Flush();
        fclose(m_File);
    }
}

void SimpleLog::Flush()
{
    fflush(m_File);

    PoolObject<Timer<void> >::Instance().SetTimeout(boost::bind(&SimpleLog::Flush, this), LOG_FLUSH_TIMEOUT);
}

void SimpleLog::Write(const char* file, int line, const char* func, const char* szFormat, ...)
{
    if(m_File == NULL)
        return;

    va_list ap;
    va_start(ap, szFormat);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm now;
    localtime_r(&tv.tv_sec, &now);

    fprintf(m_File, "[%04d-%02d-%02d %02d:%02d:%02d][%s:%d][%s]", now.tm_year+1900, now.tm_mon+1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, file, line, func);
    vfprintf(m_File, szFormat, ap);
    fprintf(m_File, "\n");

    va_end(ap);

    ShiftLog();
}

void SimpleLog::ShiftLog()
{
    time_t now = time(NULL) / 3600 * 3600;

    tm now_tm;
    localtime_r(&now, &now_tm);

    char buffer[260];
    memset(buffer, 0, 260);
    snprintf(buffer, 259, "%s/log%04d%02d%02d%02d.log%u", 
                m_Path.c_str(),
                (now_tm.tm_year+1900),
                (now_tm.tm_mon+1),
                now_tm.tm_mday,
                now_tm.tm_hour,
                Pool::Instance().GetID());

    if(access(buffer, W_OK|F_OK) == 0)
        return;

    fclose(m_File);
    m_File = fopen(buffer, "a");
}



