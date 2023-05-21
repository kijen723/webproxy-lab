# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LIB = -lpthread -lrt

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) csapp.c -c -o csapp.o

echo.o: echo.c
	$(CC) $(CFLAGS) echo.c -c -o echo.o

all: csapp.o echo.o echoclient.c echoserveri.c
	$(CC) $(CFLAGS) csapp.o echoclient.c -o echoclient $(LIB)
	$(CC) $(CFLAGS) csapp.o echo.o echoserveri.c -o echoserveri $(LIB)

clean:
	rm -f *.o echoclient echoserveri 