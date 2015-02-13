//connection.h is dependent on network.h
typedef struct {
	TCP_PROTOCOL_T type;//TCP_PROTOCOL_T from networking.h
	sslConnection sslConnection;//used for ssl connections
	int socketFD;//used for non-ssl conections
} Connection_t;

void net_setupNewConnection(Connection_t *con);

void net_freeConnection(Connection_t *input);

int net_connect(Connection_t *con, char *domain);

int net_close(Connection_t *con, char *domain);

int net_send(Connection_t *con, char *packetBuf, int dataSize);

int net_recv(Connection_t *con, char *packetBuf, int maxBufferSize);

//converts a file decriptor to a connection
void net_fileDescriptorToConnection(const int fd, Connection_t *con);
