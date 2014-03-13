#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

include ../Makefile.env

TARGET := ../bin/tweetadsd 
OBJS := 

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../bin/tweetadsd: objs/tweetadsd.o $(OBJS) ../libs/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)




