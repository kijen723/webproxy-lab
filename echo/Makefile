# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LIB = -lpthread -lrt

all: csapp.o echo.o echo_client.c echo_server.c
	$(CC) $(CFLAGS) csapp.o echo_client.c -o echo_client $(LIB)
	$(CC) $(CFLAGS) csapp.o echo.o echo_server.c -o echo_server $(LIB)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) csapp.c -c -o csapp.o

echo.o: echo.c
	$(CC) $(CFLAGS) echo.c -c -o echo.o

clean:
	rm -f *.o echo_client echo_server