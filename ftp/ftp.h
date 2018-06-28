void ftpClientState_init(ftpClientState_t *clientState, int command_fd,
		char *usernameBuffer, int usernameBufferLength,
		char *fileNameChangeBuffer, int fileNameChangeBufferLength);

int sendFtpResponse(ftpClientState_t *clientState, char *contents);

int isValidLogin(char *username, char *password);
