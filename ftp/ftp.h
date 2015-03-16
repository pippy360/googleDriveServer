
void ftp_newClientState(ftpClientState_t *clientState, int command_fd, char *usernameBuffer, int usernameBufferLength);

int sendFtpResponse(ftpClientState_t *clientState, char *contents);
