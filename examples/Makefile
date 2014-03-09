#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

include ../Makefile.env

TARGET := ../bin/server_example ../bin/client_example
OBJS := 

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../bin/server_example: objs/main.o ../libs/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)

../bin/client_example: objs/client.o ../libs/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)



