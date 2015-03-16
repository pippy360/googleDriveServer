#include <string.h>
#include "virtualFileSystem/hiredis/hiredis.h"
#include "virtualFileSystem/vfs.h"

#include "net/networking.h"

#include "ftp/ftpCommon.h"
#include "ftp/ftp.h"
#include "ftp/ftpParser.h"

#define EXAMPLE_DIR "-rw-rw-r--. 1 lilo lilo 100 Feb 26 07:08 file1\r\n"

#define MAX_PACKET_SIZE 1600
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define VALID_GREETING "220 fuck yeah you've connected ! what are you looking for...?\r\n"

#define SEVER_IP_FTP "127,0,0,1"

void openDataConnection(ftpClientState_t *clientState) {
	char strBuf1[1000]; //FIXME: hardcoded
	int tempSock = getListeningSocket("5000"); //FIXME: WHAT DO I DO WITH TEMPSOCK ?????
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
		ftpParserState_t *parserState, ftpClientState_t *clientState) {

	char strBuf1[1800]; //FIXME: hardcoded
	char tempBuffer[1800]; //FIXME: hardcoded
	long id, fileSize;
	int received;

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
		vfs_getFolderPathFromId(vfsContext, clientState->cwdId, tempBuffer,
				1000);
		sprintf(strBuf1, "257 %s\r\n", tempBuffer);
		sendFtpResponse(clientState, strBuf1);
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
		id = vfs_getIdFromPath(vfsContext, parserState->paramBuffer);
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

		send(clientState->data_fd, EXAMPLE_DIR, strlen(EXAMPLE_DIR), 0);

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
		if ((id = vfs_getIdFromPath(vfsContext, parserState->paramBuffer))
				== -1) {
			sendFtpResponse(clientState, "550 Failed to change directory.\r\n");
		} else {
			clientState->cwdId = id;
			sendFtpResponse(clientState,
					"250 Directory successfully changed.\r\n"); //success
		}
		break;
	case REQUEST_STOR:
		//get the file name
		printf("trying to store file %s\n", parserState->paramBuffer);

		//FIXME: make sure we have a connection open
		sprintf(strBuf1, "150 FILE: /%s\r\n", parserState->paramBuffer);

		sendFtpResponse(clientState, strBuf1);

		//alright start reading in the file
		while ((received = recv(clientState->data_fd, tempBuffer, 1800, 0)) > 0) {
			//printf("recv'd:--%.*s--\n", received, tempBuffer);
		}

		sendFtpResponse(clientState, "226 Transfer complete.\r\n");

		if (close(clientState->data_fd) != 0) {
			perror("close:");
		}
		if (close(clientState->data_fd2) != 0) {
			perror("close:");
		}

		break;
	default:
		sendFtpResponse(clientState, "502 Command not implemented\r\n");
		break;
	}
}

void handle_client(int client_fd) {
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
	char buffer[MAX_PACKET_SIZE], pBuffer[MAX_PACKET_SIZE], usernameBuffer[100];
	int sent, recieved;
	ftpParserState_t parserState;
	ftpClientState_t clientState;
	ftp_newParserState(&parserState, pBuffer, MAX_PACKET_SIZE);
	ftp_newClientState(&clientState, client_fd, usernameBuffer, 100);
	sent = send(client_fd, VALID_GREETING, strlen(VALID_GREETING), 0);
	while (1) {
		recieved = recv(client_fd, buffer, MAX_PACKET_SIZE, 0);
		printf("buffer: %.*s--\n", recieved, buffer);
		ftp_parsePacket(buffer, recieved, &parserState, &clientState);
		if (parserState.type == REQUEST_QUIT) {
			printf("got a quit\n");
			break;
		}
		ftp_handleFtpRequest(c, &parserState, &clientState);
	}
	close(client_fd);
	printf("connection closed\n");
}
int main(int argc, char *argv[]) {
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
			handle_client(client_fd);
			close(client_fd);
			//recv from client, now get a file from google
			exit(0);
		}
		close(client_fd); // parent doesn't need this
	}
	return 0;
}

