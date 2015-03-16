all: server.out ftpServer.out

server.out: connection.o networking.o googleAccessToken.o utils.o server.o realtimePacketParser.o createHTTPHeader.o
	gcc realtimePacketParser.o connection.o googleAccessToken.o server.o createHTTPHeader.o utils.o networking.o -lssl -lcrypto -g -o server.out

networking.o: net/networking.c
	gcc -c net/networking.c

connection.o: net/connection.c
	gcc -c net/connection.c

server.o: server.c
	gcc -c server.c

googleAccessToken.o: google/googleAccessToken.c
	gcc -c google/googleAccessToken.c

utils.o: utils.c
	gcc -c utils.c

realtimePacketParser.o: httpProcessing/realtimePacketParser.c
	gcc -c httpProcessing/realtimePacketParser.c

createHTTPHeader.o: httpProcessing/createHTTPHeader.c
	gcc -c httpProcessing/createHTTPHeader.c
	
ftpServer.out: ftpServer.o createHTTPHeader.o ftpParser.o googleUpload.o realtimePacketParser.o ftp.o vfs.o networking.o utils.o connection.o googleAccessToken.o
	gcc ftpServer.o googleUpload.o createHTTPHeader.o realtimePacketParser.o googleAccessToken.o ftp.o ftpParser.o vfs.o utils.o networking.o connection.o -lcrypto -lssl ./virtualFileSystem/hiredis/*.o -o ftpServer.out

ftpServer.o: ftpServer.c
	gcc -c ftpServer.c

ftp.o: ftp/ftp.c
	gcc -c ftp/ftp.c

ftpParser.o: ./ftp/ftpParser.o
	gcc -c ftp/ftpParser.c

vfs.o: virtualFileSystem/vfs.c
	gcc -c virtualFileSystem/vfs.c

googleUpload.o: google/googleUpload.c
	gcc -c google/googleUpload.c
	
	
