#Makefile for asign 4

CC = gcc
CFLAGS = -Wall -pedantic -g

all: mytar

mytar: mytar.c
	$(CC) $(CFLAGS) -o mytar mytar.c

clean: rm -f *.o
