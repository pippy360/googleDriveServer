all: httpServer.out ftpServer.out

httpServer.out: fileTransfer.o connection.o crypto.o networking.o googleAccessToken.o utils.o httpServer.o realtimePacketParser.o createHTTPHeader.o
	gcc realtimePacketParser.o fileTransfer.o crypto.o connection.o googleAccessToken.o httpServer.o createHTTPHeader.o utils.o networking.o -lssl -lcrypto -g -o httpServer.out

networking.o: net/networking.c
	gcc -c net/networking.c

connection.o: net/connection.c
	gcc -c net/connection.c

httpServer.o: httpServer.c
	gcc -c httpServer.c

fileTransfer.o: fileTransfer.c
	gcc -c fileTransfer.c

googleAccessToken.o: google/googleAccessToken.c
	gcc -c google/googleAccessToken.c

utils.o: utils.c
	gcc -c utils.c

realtimePacketParser.o: httpProcessing/realtimePacketParser.c
	gcc -c httpProcessing/realtimePacketParser.c

createHTTPHeader.o: httpProcessing/createHTTPHeader.c
	gcc -c httpProcessing/createHTTPHeader.c
	
ftpServer.out: ftpServer.o createHTTPHeader.o crypto.o fileTransfer.o ftpParser.o googleUpload.o realtimePacketParser.o ftp.o vfs.o networking.o utils.o connection.o googleAccessToken.o
	gcc ftpServer.o googleUpload.o createHTTPHeader.o crypto.o fileTransfer.o realtimePacketParser.o googleAccessToken.o ftp.o ftpParser.o vfs.o utils.o networking.o connection.o -lcrypto -lssl ./virtualFileSystem/hiredis/*.o -o ftpServer.out

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
	
crypto.o: crypto.c
	gcc -c crypto.c
