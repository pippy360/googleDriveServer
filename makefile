all: server.out

server.out: networking.o utils.o pushPull.o
	gcc pushPull.o utils.o networking.o -lssl -lcrypto -o server.out

networking.o: networking.c
	gcc -c networking.c

pushPull.o: pushPull.c
	gcc -c pushPull.c

utils.o: utils.c
	gcc -c utils.c
