#include <string.h>
#include "virtualFileSystem/hiredis/hiredis.h"
#include "virtualFileSystem/vfs.h"
#include "virtualFileSystem/vfsPathParser.h"

#include "commonDefines.h"

#include "net/networking.h"
#include "net/connection.h"

#include "ftp/ftpCommon.h"
#include "ftp/ftp.h"
#include "ftp/ftpParser.h"

#include "httpProcessing/commonHTTP.h"//TODO: parser states
#include "httpProcessing/realtimePacketParser.h"//TODO: parser states
#include "httpProcessing/createHTTPHeader.h"

#include "google/googleAccessToken.h"
#include "google/googleUpload.h"

#include "crypto.h"
#include "fileTransfer.h"
#include "utils.h"

#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define VALID_GREETING "220 Hey\r\n"

#define SEVER_IP_FTP "127,0,0,1"

#define STRING_BUFFER_LEN 2000
#define ENCRYPTED_BUFFER_LEN 2000
#define DECRYPTED_BUFFER_LEN 2000
#define AES_BLOCK_SIZE 16//fixme: get this from a config file



void flipBits(void* packetData, int size) {
	char* b = packetData;
	int i;
	for (i = 0; i < size; ++i) {
		b[i] = ~(b[i]);
	}
}

void openDataConnection(ftpClientState_t *clientState) {
	char strBuf1[1000]; //FIXME: hardcoded
	int tempSock = getListeningSocket("0"); //FIXME: WHAT DO I DO WITH TEMPSOCK ?????
	clientState->data_fd2 = tempSock;
	int port = getPort(tempSock);
	sprintf(strBuf1, "227 Entering Passive Mode (%s,%d,%d).\r\n", SEVER_IP_FTP,
			port >> 8, port & 255);
	sendFtpResponse(clientState, strBuf1);
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	sin_size = sizeof their_addr;
	clientState->data_fd = accept(tempSock, (struct sockaddr *) &their_addr,
			&sin_size);
	if (clientState->data_fd == -1) {
		perror("accept");
	}
	clientState->isDataConnectionOpen = 1;
}

void ftp_handleFtpRequest(redisContext *vfsContext,
		AccessTokenState_t *accessTokenState, ftpParserState_t *parserState,
		ftpClientState_t *clientState) {

	char strBuf1[STRING_BUFFER_LEN], strBuf2[STRING_BUFFER_LEN];
	//we will under report the size of the buffers to functions when using them as input buffers
	//so that we always meet the size+AES_BLOCK_SIZE requirements of the output buffer for the encrypt/decrypt functions
	//see openssl docs for more info on ecrypted buffer/decrypted buffers size requirements
	char encryptedDataBuffer[ENCRYPTED_BUFFER_LEN + AES_BLOCK_SIZE];
	char decryptedDataBuffer[DECRYPTED_BUFFER_LEN + AES_BLOCK_SIZE];
	int tempOutputSize;
	long id, fileSize;
	int received, dataLength;
	GoogleUploadState_t fileState;
	Connection_t googleCon;
	headerInfo_t hInfo;
	parserState_t googleParserState;
	CryptoState_t encryptionState, decryptionState;
	vfsPathParserState_t vfsParserState;

	char encryptionStarted;

	switch (parserState->type) {
	case REQUEST_USER:
		strcpy(clientState->usernameBuffer, parserState->paramBuffer);
		sendFtpResponse(clientState, "331 User name okay, need password.\r\n");
		clientState->loggedIn = 0;
		break;
	case REQUEST_PASS:
		if (isValidLogin(clientState->usernameBuffer,
				parserState->paramBuffer)) {
			sendFtpResponse(clientState, "230 User logged in, proceed.\r\n");
			clientState->loggedIn = 1;
		} else {
			sendFtpResponse(clientState,
					"430 Invalid username or password\r\n");
		}
		break;
	case REQUEST_SYST:
		sendFtpResponse(clientState, "215 UNIX Type: L8\r\n");
		break;
	case REQUEST_PWD:
		printf("PWD called, the CWD is %d\n", clientState->cwdId);
		vfs_getDirPathFromId(vfsContext, clientState->cwdId, strBuf1,
				STRING_BUFFER_LEN);
		sprintf(strBuf2, "257 \"%s\"\r\n", strBuf1);
		sendFtpResponse(clientState, strBuf2);
		break;
	case REQUEST_TYPE:
		if (strcmp(parserState->paramBuffer, "I") == 0) {
			clientState->transferType = FTP_TRANSFER_TYPE_I;
			sendFtpResponse(clientState, "200 Switching to Binary mode..\r\n");
		} else {
			sendFtpResponse(clientState, "200 Switching to Binary mode..\r\n");
			//sendFtpResponse(clientState, "504 Command not implemented for that parameter.\r\n");
		}
		break;
	case REQUEST_PASV:
		if (clientState->isDataConnectionOpen == 1) {
			printf("there was an open connection, it's closed\n");
			close(clientState->data_fd);
			close(clientState->data_fd2);
			clientState->isDataConnectionOpen = 0;
		}
		openDataConnection(clientState);
		break;
	case REQUEST_SIZE:
		vfs_parsePath(vfsContext, &vfsParserState, parserState->paramBuffer, strlen(parserState->paramBuffer), clientState->cwdId);
		id = vfsParserState.id;
		if ((fileSize = vfs_getFileSizeFromId(vfsContext, id)) == -1) {
			sendFtpResponse(clientState, "550 Could not get file size.\r\n");
		} else {
			sprintf(strBuf1, "213 %ld\r\n", fileSize);
			sendFtpResponse(clientState, strBuf1); //TODO:
		}
		break;
	case REQUEST_LIST:
		sendFtpResponse(clientState,
				"150 Here comes the directory listing.\r\n");
		int i = 0;
		char *dirList = vfs_listUnixStyle(vfsContext, clientState->cwdId);

		send(clientState->data_fd, dirList, strlen(dirList), 0);

		for (i = 0; i < 2000000; i++)
			;

		if (close(clientState->data_fd) != 0) {
			perror("close:");
		}

		if (close(clientState->data_fd2) != 0) {
			perror("close2:");
		}
		sendFtpResponse(clientState, "226 Directory send OK.\r\n");
		break;
	case REQUEST_CWD:
		vfs_parsePath(vfsContext, &vfsParserState, parserState->paramBuffer, strlen(parserState->paramBuffer), clientState->cwdId);
		if(vfsParserState.isExistingObject && vfsParserState.isDir){
			clientState->cwdId = vfsParserState.id;
			sendFtpResponse(clientState,"250 Directory successfully changed.\r\n"); //success
		}else{
			sendFtpResponse(clientState, "550 Failed to change directory.\r\n");
		}
		break;
	case REQUEST_MKD:
		vfs_mkdir(vfsContext, clientState->cwdId, parserState->paramBuffer);
		sendFtpResponse(clientState, "257 MKDIR done\r\n");
		break;
	case REQUEST_STOR:
		//get the file name
		printf("trying to store file %s\n", parserState->paramBuffer);
		//sprintf(strBuf1, "\"title\": \"%s\"", parserState->paramBuffer);
		sprintf(strBuf1, "\"title\": \"%s\"", "nuffin.bin");

		//check if the name is valid and get it's type

		googleUpload_init(&googleCon, accessTokenState, strBuf1, "image/png");

		//FIXME: make sure we have a connection open
		sprintf(strBuf1, "150 FILE: /%s\r\n", parserState->paramBuffer);
		sendFtpResponse(clientState, strBuf1);

		//alright start reading in the file
		startEncryption(&encryptionState, "phone");
		while ((received = recv(clientState->data_fd, decryptedDataBuffer, DECRYPTED_BUFFER_LEN, 0)) > 0) {

			updateEncryption(&encryptionState, decryptedDataBuffer, received, encryptedDataBuffer, &tempOutputSize);
			googleUpload_update(&googleCon, encryptedDataBuffer, tempOutputSize);
			//printf("recv'd:--%.*s--\n", received, tempBuffer);
		}
		finishEncryption(&encryptionState, decryptedDataBuffer, received, encryptedDataBuffer, &tempOutputSize);
		googleUpload_update(&googleCon, encryptedDataBuffer, tempOutputSize);

		googleUpload_end(&googleCon, &fileState);
		vfs_createFile(vfsContext, clientState->cwdId, parserState->paramBuffer,
				1000, fileState.id, fileState.webUrl, fileState.apiUrl);
		sendFtpResponse(clientState, "226 Transfer complete.\r\n");

		if (close(clientState->data_fd) != 0) {
			perror("close:");
		}
		if (close(clientState->data_fd2) != 0) {
			perror("close:");
		}

		break;
	case REQUEST_RETR:
		//FIXME: make sure we have a connection open
		//send a get request to the and then continue the download
		//take the download out to it's own file
		vfs_parsePath(vfsContext, &vfsParserState, parserState->paramBuffer, strlen(parserState->paramBuffer), clientState->cwdId);
		id = vfsParserState.id;
		vfs_getFileWebUrl(vfsContext, id, strBuf1, 2000);
		printf("the url of the file they're looking for is: %s\n", strBuf1);


		startFileDownload(strBuf1, 0, 0, 0, 0, &googleCon, &hInfo, &googleParserState,
				getAccessTokenHeader(accessTokenState));
		sendFtpResponse(clientState, "150 about to send file\r\n");
		encryptionStarted = 0;
		while (1) {
			received = updateFileDownload(&googleCon, &hInfo,
					&googleParserState, encryptedDataBuffer, ENCRYPTED_BUFFER_LEN, &dataLength, "");
			if(!encryptionStarted){
				startDecryption(&(decryptionState), "phone", NULL);
				encryptionStarted = 1;
			}
			if (received == 0) {
				//finishDecryption(&decryptionState, tempBuffer, 0, tempBuffer2, &tempBuffer2OutputSize);
				//send(clientState->data_fd, tempBuffer2, tempBuffer2OutputSize, 0);
				break;
			}
			updateDecryption(&decryptionState, encryptedDataBuffer, dataLength, decryptedDataBuffer, &tempOutputSize);
			send(clientState->data_fd, decryptedDataBuffer, tempOutputSize, 0);
		}
		sendFtpResponse(clientState, "226 Transfer complete.\r\n");
		if (close(clientState->data_fd) != 0) {
			perror("close:");
		}
		if (close(clientState->data_fd2) != 0) {
			perror("close:");
		}
		//FIXME: finishFileDownload();

		break;
	case REQUEST_RNFR:
		//buffer the command
		//check if the file exists
		sprintf(clientState->fileNameChangeBuffer, "%s", parserState->paramBuffer);
		sendFtpResponse(clientState, "350 RNFR accepted. Please supply new name for RNTO.\r\n");
		break;
	case REQUEST_RNTO:
		//rename the actual file
		//TODO: FIX THIS COMMAND TO SHOW THE FILENAMES
		printf("mv %s %s\n", clientState->fileNameChangeBuffer, parserState->paramBuffer);
		vfs_mv(vfsContext, clientState->cwdId, clientState->fileNameChangeBuffer, parserState->paramBuffer);
		sendFtpResponse(clientState, "250 renamed\r\n");
		break;
	case REQUEST_DELE:
		//here:
		//vfs_deleteObjectWithPath(vfsContext, path, cwd);
		vfs_deleteObjectWithPath(vfsContext, parserState->paramBuffer, clientState->cwdId);
		sendFtpResponse(clientState, "250 file was taken out back and shot.\r\n");
		break;
	case REQUEST_CDUP:
		clientState->cwdId = vfs_getDirParent(vfsContext, clientState->cwdId);
		sendFtpResponse(clientState,"250 Directory successfully changed.\r\n"); //success
		break;
	case REQUEST_RMD:
		vfs_deleteObjectWithPath(vfsContext, parserState->paramBuffer, clientState->cwdId);
		sendFtpResponse(clientState,"250 directory killed.\r\n"); //success
		break;
	default:
		sendFtpResponse(clientState, "502 Command not implemented.\r\n");
		break;
	}
}

void handle_client(int client_fd, AccessTokenState_t *stateStruct) {
	unsigned int j;
	redisContext *c;
	redisReply *reply;
	const char *hostname = "127.0.0.1";
	int port = 6379;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout(hostname, port, timeout);
	if (c == NULL || c->err) {
		if (c) {
			printf("REDIS Connection error: %s\n", c->errstr);
			redisFree(c);
		} else {
			printf("REDIS Connection error: can't allocate redis context\n");
		}
		exit(1);
	}
	char buffer[MAX_PACKET_SIZE], pBuffer[MAX_PACKET_SIZE], usernameBuffer[100], fileNameChangeBuffer[1000];
	int sent, recieved;
	ftpParserState_t parserState;
	ftpClientState_t clientState;
	ftp_newParserState(&parserState, pBuffer, MAX_PACKET_SIZE);
	ftp_newClientState(&clientState, client_fd, usernameBuffer, 100, fileNameChangeBuffer, 1000);
	sent = send(client_fd, VALID_GREETING, strlen(VALID_GREETING), 0);
	while (1) {
		if ((recieved = recv(client_fd, buffer, MAX_PACKET_SIZE, 0)) == 0) {
			break;
		}
		printf("buffer: %.*s--\n", recieved, buffer);
		ftp_parsePacket(buffer, recieved, &parserState, &clientState);
		if (parserState.type == REQUEST_QUIT) {
			printf("got a quit\n");
			break;
		}
		ftp_handleFtpRequest(c, stateStruct, &parserState, &clientState);
	}
	close(client_fd);
	printf("connection closed\n");
}

int main(int argc, char *argv[]) {
	AccessTokenState_t stateStruct;
	gat_init_googleAccessToken(&stateStruct);
	int sockfd = getListeningSocket(SERVER_LISTEN_PORT);
	int client_fd;
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];
	printf("server: waiting for connections...\n");
	while (1) { // main accept() loop
		sin_size = sizeof their_addr;
		client_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if (client_fd == -1) {
			perror("accept");
			continue;
		}
		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			handle_client(client_fd, &stateStruct);
			close(client_fd);
			//recv from client, now get a file from google
			exit(0);
		}
		close(client_fd); // parent doesn't need this
	}
	return 0;
}

