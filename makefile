all: httpServer.out ftpServer.out

httpServer.out: fileTransfer.o connection.o crypto.o networking.o googleAccessToken.o utils.o httpServer.o realtimePacketParser.o createHTTPHeader.o
	gcc -g -O0 realtimePacketParser.o fileTransfer.o crypto.o connection.o googleAccessToken.o httpServer.o createHTTPHeader.o utils.o networking.o -lssl -lcrypto -g -o httpServer.out

networking.o: net/networking.c
	gcc -g -O0 -c net/networking.c

connection.o: net/connection.c
	gcc -g -O0 -c net/connection.c

httpServer.o: httpServer.c
	gcc -g -O0 -c httpServer.c

fileTransfer.o: fileTransfer.c
	gcc -g -O0 -c fileTransfer.c

googleAccessToken.o: google/googleAccessToken.c
	gcc -g -O0 -c google/googleAccessToken.c

utils.o: utils.c
	gcc -g -O0 -c utils.c

realtimePacketParser.o: httpProcessing/realtimePacketParser.c
	gcc -g -O0 -c httpProcessing/realtimePacketParser.c

createHTTPHeader.o: httpProcessing/createHTTPHeader.c
	gcc -g -O0 -c httpProcessing/createHTTPHeader.c
	
ftpServer.out:  ftpServer.o vfsPathParser.o createHTTPHeader.o crypto.o fileTransfer.o ftpParser.o googleUpload.o realtimePacketParser.o ftp.o vfs.o networking.o utils.o connection.o googleAccessToken.o
	gcc -g -O0 ftpServer.o vfsPathParser.o googleUpload.o createHTTPHeader.o crypto.o fileTransfer.o realtimePacketParser.o googleAccessToken.o ftp.o ftpParser.o vfs.o utils.o networking.o connection.o -lcrypto -lssl ./virtualFileSystem/hiredis/*.o -o ftpServer.out

ftpServer.o: ftpServer.c
	gcc -g -O0 -c ftpServer.c

ftp.o: ftp/ftp.c
	gcc -g -O0 -c ftp/ftp.c

ftpParser.o: ./ftp/ftpParser.c
	gcc -g -O0 -c ftp/ftpParser.c

vfs.o: virtualFileSystem/vfs.c 
	gcc -g -O0 -c virtualFileSystem/vfs.c

vfsPathParser.o: virtualFileSystem/vfsPathParser.c
	gcc -g -O0 -c virtualFileSystem/vfsPathParser.c

googleUpload.o: google/googleUpload.c
	gcc -g -O0 -c google/googleUpload.c
	
crypto.o: crypto.c
	gcc -g -O0 -c crypto.c
