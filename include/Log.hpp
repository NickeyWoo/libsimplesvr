/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-20
 *
--*/
#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <utility>
#include <vector>
#include <string>
#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>

extern const char* safe_strerror(int error);

#ifdef DEBUG
    #define LOG(...)                                                                            \
        do {                                                                                    \
            struct timeval tv;                                                                  \
            gettimeofday(&tv, NULL);                                                            \
            struct tm now;                                                                      \
            localtime_r(&tv.tv_sec, &now);                                                      \
                                                                                                \
            printf("[%04d-%02d-%02d %02d:%02d:%02d][%s:%d][%s]",                                \
                    now.tm_year+1900, now.tm_mon+1, now.tm_mday,                                \
                    now.tm_hour, now.tm_min, now.tm_sec,                                        \
                    __FILE__, __LINE__, __FUNCTION__);                                          \
            printf(__VA_ARGS__);                                                                \
            printf("\n");                                                                       \
            PoolObject< ::SimpleLog>::Instance().Write(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
        } while(0)

    #define TRACE_LOG(...)                                                                      \
        do {                                                                                    \
            struct timeval tv;                                                                  \
            gettimeofday(&tv, NULL);                                                            \
            struct tm now;                                                                      \
            localtime_r(&tv.tv_sec, &now);                                                      \
                                                                                                \
            printf("[%04d-%02d-%02d %02d:%02d:%02d][%s:%d][%s]",                                \
                    now.tm_year+1900, now.tm_mon+1, now.tm_mday,                                \
                    now.tm_hour, now.tm_min, now.tm_sec,                                        \
                    __FILE__, __LINE__, __FUNCTION__);                                          \
            printf(__VA_ARGS__);                                                                \
            printf("\n");                                                                       \
        } while(0)

#else
    #define LOG(...)                                                                            \
        PoolObject< ::SimpleLog>::Instance().Write(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

    #define TRACE_LOG(...)
#endif

#define LOG_FLUSH_TIMEOUT       1000

class SimpleLog :
    public boost::noncopyable
{
public:
    SimpleLog();
    ~SimpleLog();

    bool Initialize(std::string path);
    void Close();
    void Flush();

    void Write(const char* file, int line, const char* func, const char* szFormat, ...) __attribute__ ((format(printf, 5, 6)));

private:
    void ShiftLog();

    FILE* m_File;
    std::string m_Path;
};

#endif // define __LOG_HPP__
