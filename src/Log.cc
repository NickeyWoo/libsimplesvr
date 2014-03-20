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
#include <pthread.h>
#include "Log.hpp"

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




