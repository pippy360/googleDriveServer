typedef struct {
	protocol_t type;
	sslConnection *sslConnection;
	int httpSocketFD;
} Connection_t;

void net_setNewConnection(Connection_t *httpConnection);

void net_freeConnection(Connection_t *input);

int net_connect(Connection_t *httpConnection, char *domain);

int net_close(Connection_t *httpConnection, char *domain);

int net_send(Connection_t *httpConnection, char *packetBuf, int dataSize);

int net_recv(Connection_t *httpConnection, char *packetBuf, int maxBufferSize);

//converts a file decriptor to a connection
void net_FileDescriptorToConnection(const int fd, Connection_t *httpCon);
