#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

include ../Makefile.env

TARGET := ../bin/server_example ../bin/client_example
OBJS := objs/my.pb.o

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../bin/server_example: objs/main.o $(OBJS) ../libs/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)

../bin/client_example: objs/client.o $(OBJS)
	$(CXX) $^ -o $@ $(LIBS)



