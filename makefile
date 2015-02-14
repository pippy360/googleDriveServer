all: server.out

server.out: connection.o networking.o utils.o server.o realtimePacketParser.o createHTTPHeader.o
	gcc realtimePacketParser.o connection.o server.o createHTTPHeader.o utils.o networking.o -lssl -lcrypto -g -o server.out

networking.o: net/networking.c
	gcc -c net/networking.c

connection.o: net/connection.c
	gcc -c net/connection.c

server.o: server.c
	gcc -c server.c

utils.o: utils.c
	gcc -c utils.c

realtimePacketParser.o: httpProcessing/realtimePacketParser.c
	gcc -c httpProcessing/realtimePacketParser.c

createHTTPHeader.o: httpProcessing/createHTTPHeader.c
	gcc -c httpProcessing/createHTTPHeader.c
