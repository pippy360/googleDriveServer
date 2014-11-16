all: server.out

server.out: networking.o utils.o pushPull.o google.o parser.o
	gcc pushPull.o parser.o utils.o networking.o google.o -lssl -lcrypto -g -o server.out

networking.o: networking.c
	gcc -c networking.c

pushPull.o: pushPull.c
	gcc -c pushPull.c

utils.o: utils.c
	gcc -c utils.c

google.o: google.c
	gcc -c google.c

parser.o: parser.c
	gcc -c parser.c
