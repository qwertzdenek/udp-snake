CC=gcc
LD=ld
LDFLAGS=-lc -pthread
CFLAGS=-pedantic -Wall -O2 -std=gnu99

all:	clean server

server:	
	${CC} -c ${CFLAGS} -o game.o game.c
	${CC} -c ${CFLAGS} -o server.o server.c
	${CC} -c ${CFLAGS} -o queue.o queue.c
	${CC} -o snake-server game.o server.o queue.o ${LDFLAGS}

clean: 
	rm -f snake-server *.o
