#include <string.h>
#include "virtualFileSystem/hiredis/hiredis.h"
#include "virtualFileSystem/vfs.h"

#include "ftp/ftpCommon.h"
#include "ftp/ftp.h"

#define EXAMPLE_DIR "-rw-rw-r--. 1 lilo lilo 100 Feb 26 07:08 file1\r\n"


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


int main(int argc, char **argv) {
	//something
}

