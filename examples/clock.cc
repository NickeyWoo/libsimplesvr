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
#include <boost/format.hpp>
#include "PoolObject.hpp"
#include "Clock.hpp"

void f3()
{
    usleep(500000);
}

void f2()
{
    usleep(10000);
}

void f1()
{
    sleep(1);
}

int main(int argc, char* argv[])
{
    CLOCK_BEGIN("startup");
    f1();
    CLOCK_TRACE("f1");
    f2();
    CLOCK_TRACE("f2");

    printf("%s", CLOCK_DUMP().c_str());
    CLOCK_CLEAR();

    CLOCK_TRACE("start f3");
    f3();
    CLOCK_TRACE("f3");

    printf("span: %.02f\n", CLOCK_TIMESPAN());

    printf("%s", CLOCK_DUMP().c_str());

    return 0;
}


