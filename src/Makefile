#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

include ../Makefile.env

TARGET := ../libs/libsimplesvr.a
OBJS := objs/EPoll.o objs/IOBuffer.o objs/Configure.o objs/Clock.o

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../libs/libsimplesvr.a: $(OBJS)
	$(AR) -r $@ $^


