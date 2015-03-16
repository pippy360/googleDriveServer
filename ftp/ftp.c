/*
 *
 *
 *
 * a parserState must have a valid request and parameter before it can be passed into any of these functions
 *
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "../net/networking.h"
#include "ftpCommon.h"


#define SEVER_IP_FTP "127,0,0,1"

#define MAX_FILE_NAME 1000;


//FIXME:
int isValidLogin(char *username, char *password) {
	if (strcmp(username, "tom") == 0 && strcmp(password, "wootwoot") == 0)
		return 1;
	else
		return 0;
}

int sendFtpResponse(ftpClientState_t *clientState, char *contents) {
	printf("response: %s\n", contents);
	int status = send(clientState->command_fd, contents, strlen(contents), 0);
	if (status <= 0) {
		perror("sendFtpResponse send");
	}
	return status;
}

void ftp_newClientState(ftpClientState_t *clientState, int command_fd,
		char *usernameBuffer, int usernameBufferLength) {

	clientState->command_fd = command_fd;
	clientState->usernameBuffer = usernameBuffer;
	clientState->usernameBufferLength = usernameBufferLength;
	clientState->transferType = FTP_TRANSFER_TYPE_I;
	clientState->loggedIn = 0;
	clientState->data_fd = -1;
	clientState->data_fd2 = -1;
	clientState->isDataConnectionOpen = 0;
	clientState->cwdId = 0;
}

void openDataConnection(ftpClientState_t *clientState) {
	char strBuf1[1000]; //FIXME: hardcoded
	int tempSock = get_listening_socket("5000"); //FIXME: WHAT DO I DO WITH TEMPSOCK ?????
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

void sendFile(ftpClientState_t *clientState) {
	FILE *ifp, *ofp;
	char *mode = "r";
	char outputFilename[] = "out.list";

	ifp = fopen("output.txt", mode);

	if (ifp == NULL) {
		fprintf(stderr, "Can't open input file in.list!\n");
		exit(1);
	}
	char buffer[100000];
	fread(buffer, 10000, 1, ifp);
	send(clientState->data_fd, buffer, strlen(buffer), 0);
	close(clientState->data_fd);
}


