void ftp_newClientState(ftpClientState_t *clientState, int command_fd,
		char *usernameBuffer, int usernameBufferLength,
		char *fileNameChangeBuffer, int fileNameChangeBufferLength);

int sendFtpResponse(ftpClientState_t *clientState, char *contents);
