CC = gcc
CFLAGS = -O2 -Wall -I .

LIB = -lpthread

all: echoserver echoclient

echoserver: echoserver.c csapp.o
	$(CC) $(CFLAGS) -o echoserver echoserver.c csapp.o $(LIB)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

echoclient: echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB)

clean:
	rm -f *.o echoserver *~
	rm -f *.o echoclient *~
