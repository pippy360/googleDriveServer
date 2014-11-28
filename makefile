all: server.out

server.out: networking.o utils.o pushPull.o googleAccessToken.o parser.o realtimePacketParser.o
	gcc realtimePacketParser.o pushPull.o parser.o utils.o networking.o googleAccessToken.o -lssl -lcrypto -g -o server.out

networking.o: net/networking.c
	gcc -c net/networking.c

pushPull.o: pushPull.c
	gcc -c pushPull.c

utils.o: utils.c
	gcc -c utils.c

googleAccessToken.o: googleAccessToken.c
	gcc -c googleAccessToken.c

parser.o: parser.c
	gcc -c parser.c

realtimePacketParser.o: net/realtimePacketParser.c
	gcc -c net/realtimePacketParser.c
