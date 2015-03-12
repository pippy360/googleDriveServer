all: test.out

test.out: googleAccessToken.o googleAccessToken_tests.o connection.o networking.o utils.o realtimePacketParser.o createHTTPHeader.o
	gcc realtimePacketParser.o googleAccessToken.o googleAccessToken_tests.o connection.o createHTTPHeader.o utils.o networking.o -lssl -lcrypto -g -o googleAccessToken_test.out

googleAccessToken.o: ../google/googleAccessToken.c
	gcc -c ../google/googleAccessToken.c

networking.o: ../net/networking.c
	gcc -c ../net/networking.c

connection.o: ../net/connection.c
	gcc -c ../net/connection.c

googleAccessToken_tests.o: googleAccessToken_tests.c
	gcc -c googleAccessToken_tests.c

utils.o: ../utils.c
	gcc -c ../utils.c

realtimePacketParser.o: ../httpProcessing/realtimePacketParser.c
	gcc -c ../httpProcessing/realtimePacketParser.c

createHTTPHeader.o: ../httpProcessing/createHTTPHeader.c
	gcc -c ../httpProcessing/createHTTPHeader.c

