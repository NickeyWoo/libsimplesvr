#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

include ../Makefile.env

TARGET := ../bin/tcpserviced ../bin/log ../bin/udpserviced ../bin/clock ../bin/mysqlpool ../bin/tcpclient
OBJS := 

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../bin/mysqlpool: objs/mysqlpool.o $(OBJS) ../lib/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS) $(MYSQL_LIB) $(HIREDIS_LIB)

../bin/clock: objs/clock.o $(OBJS) ../lib/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)

../bin/udpserviced: objs/udpservice.o $(OBJS) ../lib/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)

../bin/tcpserviced: objs/tcpservice.o $(OBJS) ../lib/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)

../bin/tcpclient: objs/tcpclient.o $(OBJS) ../lib/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)

../bin/log: objs/log.o ../lib/libsimplesvr.a
	$(CXX) $^ -o $@ $(LIBS)



