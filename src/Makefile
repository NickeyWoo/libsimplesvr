#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

include ../Makefile.env

TARGET := ../lib/libsimplesvr.a
OBJS := objs/EPoll.o objs/Configure.o objs/Clock.o objs/Server.o objs/IOBuffer.o objs/Pool.o objs/Log.o objs/ConnectionPool.o

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../lib/libsimplesvr.a: $(OBJS)
	$(AR) -r $@ $^


