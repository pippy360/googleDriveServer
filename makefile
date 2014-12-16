all: server.out

server.out: networking.o utils.o pushPull.o googleAccessToken.o parser.o googleUpload.o realtimePacketParser.o createHTTPHeader.o
	gcc realtimePacketParser.o pushPull.o createHTTPHeader.o parser.o utils.o networking.o googleUpload.o googleAccessToken.o -lssl -lcrypto -g -o server.out

networking.o: net/networking.c
	gcc -c net/networking.c

pushPull.o: pushPull.c
	gcc -c pushPull.c

utils.o: utils.c
	gcc -c utils.c

googleAccessToken.o: googleAccessToken.c
	gcc -c googleAccessToken.c

googleUpload.o: googleUpload.c
	gcc -c googleUpload.c

parser.o: parser.c
	gcc -c parser.c

realtimePacketParser.o: httpProcessing/realtimePacketParser.c
	gcc -c httpProcessing/realtimePacketParser.c

createHTTPHeader.o: httpProcessing/createHTTPHeader.c
	gcc -c httpProcessing/createHTTPHeader.c
