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

Log::Log() :
	m_File(NULL)
{
}

Log::~Log()
{
	Close();
}

bool Log::Initialize(std::string path)
{
	m_Path = path;
	if(m_Path.empty())
		m_Path = std::string(".");

	std::string strLog = (boost::format("%s/runlog_%u_0.log") % m_Path % Pool::Instance().GetID()).str();
	m_File = fopen(strLog.c_str(), "a");
	if(m_File == NULL)
		return false;
	return true;
}

void Log::Close()
{
	if(m_File)
	{
		Flush();
		fclose(m_File);
	}
}

void Log::Flush()
{
	fflush(m_File);
}

void Log::Write(const char* file, int line, const char* szFormat, ...)
{
	if(m_File == NULL)
		return;

	va_list ap;
	va_start(ap, szFormat);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm now;
	localtime_r(&tv.tv_sec, &now);

	fprintf(m_File, "[%04d-%02d-%02d %02d:%02d:%02d][%s:%d]", now.tm_year+1900, now.tm_mon+1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, file, line);
	vfprintf(m_File, szFormat, ap);
	fprintf(m_File, "\n");

	va_end(ap);

	ShiftLog();
}

void Log::ShiftLog()
{
	int fd = fileno(m_File);
	if(fd == -1)
		return;

	struct stat buff;
	if(fstat(fd, &buff) != -1 &&
		buff.st_size >= MAX_LOGFILE_SIZE)
	{
		fflush(m_File);
		fclose(m_File);

		std::string file;
		for(int i=MAX_LOGFILE_COUNT-1; i>=0; --i)
		{
			file = (boost::format("%s/runlog_%u_%d.log") % m_Path % Pool::Instance().GetID() % i).str();
			if(0 != access(file.c_str(), R_OK|W_OK))
				continue;

			if(i == MAX_LOGFILE_COUNT-1)
				unlink(file.c_str());
			else
			{
				std::string newfile = (boost::format("%s/runlog_%u_%d.log") % m_Path % Pool::Instance().GetID() % (i + 1)).str();
				rename(file.c_str(), newfile.c_str());
			}
		}
		m_File = fopen(file.c_str(), "a");
	}
}



