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
#include <sys/time.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "utility.hpp"

int main(int argc, char* argv[])
{
	if(argc < 4)
	{
		printf("usage: client_example [ip] [port] [loop]\n");
		return -1;
	}

#ifdef VERSION_OLD
	int fd = socket(PF_INET, SOCK_DGRAM|SOCK_CLOEXEC, 0);
#else
	int fd = socket(PF_INET, SOCK_DGRAM|SOCK_CLOEXEC, 0);
#endif
	if(fd == -1)
	{
		printf("error: create socket fail, %s.\n", strerror(errno));
		return -1;
	}

	sockaddr_in addr;
	bzero(&addr, sizeof(sockaddr_in));
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));
	
	char buffer[65535];
	uint32_t loop = strtoul(argv[3], NULL, 10);

	uint32_t pos = 0;
	uint16_t* pLen = (uint16_t*)(buffer + pos);
	//*pLen = htons(random() % 50);
	*pLen = htons(1);
	pos += sizeof(uint16_t);

	printf("search %d random uin:\n", ntohs(*pLen));

	for(uint16_t j=0; j<ntohs(*pLen); ++j)
	{
		uint32_t* pUin = (uint32_t*)(buffer + pos);
		*pUin = htonl(random() % 600000);
		pos += sizeof(uint32_t);
	}

	timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	for(uint32_t i=0; i<loop; ++i)
	{
		char recvbuf[65535];

		ssize_t sc = sendto(fd, buffer, pos, 0, (sockaddr*)&addr, sizeof(sockaddr_in));
		if(sc == -1)
		{
			printf("error: send fail, %s.\n", strerror(errno));
			continue;
		}

		ssize_t rc = recvfrom(fd, recvbuf, 65535, 0, NULL, 0);
		if(rc == -1)
		{
			printf("error: recv fail, %s.\n", strerror(errno));
			continue;
		}

		uint16_t* pRecvLen = (uint16_t*)recvbuf;
		if(*pRecvLen > 50)
		{
			printf("error: return num more than 50.");
			HexDump(buffer, pos, NULL);
			HexDump(recvbuf, rc, NULL);
			break;
		}
	}
	gettimeofday(&tv2, NULL);

	double time = ((tv2.tv_sec * 1000000 + tv2.tv_usec) - (tv1.tv_sec * 1000000 + tv1.tv_usec));
	printf("loop(%u), time: %.03fs\n", loop, time/1000000);

	close(fd);
	return 0;
}


