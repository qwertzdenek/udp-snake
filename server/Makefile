# UDP Snake Makefile, Zdeněk Janeček

CC=gcc
LD=ld
LDFLAGS=-lc -pthread
CFLAGS=-g -pedantic -Wall -O2 -std=gnu99

all:	clean server

server:	
	${CC} -c ${CFLAGS} -o game.o game.c
	${CC} -c ${CFLAGS} -o server.o server.c
	${CC} -c ${CFLAGS} -o queue.o queue.c
	${CC} -c ${CFLAGS} -o server-one.o server-one.c
	${CC} -o snake-server game.o server.o queue.o server-one.o ${LDFLAGS}

clean: 
	rm -f snake-server *.o
