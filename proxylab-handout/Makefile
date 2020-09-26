# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread
PXY = pxydrive-tutorial/pxy/pxydrive.py


all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h cache.h
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

proxy: proxy.o cache.o csapp.o
	$(CC) $(CFLAGS) proxy.o cache.o csapp.o -o proxy $(LDFLAGS)

test-pxy-1: proxy
	$(PXY) -p ./$< -f pxydrive-tutorial/s01-basic-fetch.cmd;

test-pxy-2: proxy
	$(PXY) -p ./$< -f pxydrive-tutorial/s02-basic-request.cmd;

test-pxy-3: proxy
	$(PXY) -p ./$< -f pxydrive-tutorial/s03-overrun.cmd;

test: test-pxy-1 test-pxy-2 test-pxy-3

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

