#
# Simple Server Library
# Author: NickWu
# Date: 2014-03-01
#

all:
	$(MAKE) -C src
	$(MAKE) -C examples

clean:
	$(MAKE) -C src clean
	$(MAKE) -C examples clean


