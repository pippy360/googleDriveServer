all: server.out

server.out: networking.o utils.o pushPull.o
	gcc pushPull.o utils.o -o server.out

networking.o: networking.c
	gcc -c networking.c -lssl

pushPull.o: pushPull.c
	gcc -c pushPull.c

utils.o: utils.c
	gcc -c utils.c