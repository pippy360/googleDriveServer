typedef struct {
	protocol_t type;
	sslConnection *sslConnection;
	int httpSocketFD;
} httpConnection_t;

void connectByUrl(char *inputUrl, httpConnection_t *httpConnection);

void set_new_httpConnection(httpConnection_t *httpConnection);

void freeHttpConnection(httpConnection_t *input);

void getHttpConnectionByUrl(char *inputUrl, httpConnection_t *httpConnection, char **domain);

int connect_http(httpConnection_t *httpConnection, char *domain);

int close_http(httpConnection_t *httpConnection, char *domain);

int send_http(httpConnection_t *httpConnection, char *packetBuf, int dataSize);

int recv_http(httpConnection_t *httpConnection, char *packetBuf, int maxBufferSize);

void getHttpConnectionByFileDescriptor(int fd, httpConnection_t *httpCon);
