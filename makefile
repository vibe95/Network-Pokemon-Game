CC = gcc
PORT=20010
CFLAGS= -DPORT=\$(PORT) -g -Wall

all: game SampleServer

SampleServer: SampleServer.o
	${CC} ${CFLAGS} -o $@ SampleServer.o

game: game.o
	${CC} ${CFLAGS} -o $@ game.o

%.o: %.c
	${CC} ${CFLAGS}  -c $<

clean:
	rm *.o game