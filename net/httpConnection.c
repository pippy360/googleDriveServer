#include <stdio.h>
#include <stdlib.h>
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

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "networking.h"
#include "../parser.c"

/*
 *
 *
 * this page contains http/https independent connect/send/recv functions
 *
 *
 */

typedef struct {
	protocol_t type;
	sslConnection_t *sslConnection;
	int httpSocketFD;

} httpConnection_t;

void connectByUrl(char *inputUrl, httpConnection_t *httpConnection){
	set_new_httpConnection(httpConnection);
	char *domain;//TODO: this needs to be fixed when you fix parse url
	getHttpConnectionByUrl(inputUrl, httpConnection, &domain);
	connect_http(domain, httpConnection);
}

void set_new_httpConnection(httpConnection_t *httpConnection){
	memset(httpConnection, 0, sizeof(httpConnection_t));
}

void freeHttpConnection(httpConnection_t *input){
	//todo
	//call the free sslConnection rather than freeing it here
}

void getHttpConnectionByUrl(char *inputUrl, httpConnection_t *httpConnection, domain){
	protocol_t type;
	char *domain, *fileUrl;
	parseUrl(inputUrl, &type, &domain, &fileUrl);
	if (type == PROTO_HTTP){
		httpConnection->type = PROTO_HTTP;
	}else if(type == PROTO_HTTPS){
		httpConnection->type = PROTO_HTTPS;
	}
}

int connect_http(httpConnection_t *httpConnection, char *domain){
	if (httpConnection->type == PROTO_HTTP){
		httpConnection->httpSocketFD = set_up_tcp_connection(domain, "80");
	}else if(type == PROTO_HTTPS){
		httpConnection->sslConnection = sslConnect(domain, "443");
	}
}

int close_http(httpConnection_t *httpConnection, char *domain){
	if (httpConnection->type == PROTO_HTTP){
		httpConnection->httpSocketFD = set_up_tcp_connection(domain, "80");
	}else if(type == PROTO_HTTPS){
		httpConnection->sslConnection = sslConnect(domain, "443");
	}
}

int send_http(httpConnection_t *httpConnection, char *packetBuf, int dataSize){
	if (httpConnection->type == PROTO_HTTP){
		return send(httpConnection->httpSocketFD, packetBuf, dataSize, 0);
	}else if(type == PROTO_HTTPS){
		return SSL_write (httpConnection->sslHandle, packetBuf, dataSize);
	}
}

int recv_http(httpConnection_t *httpConnection, char *packetBuf, int maxBufferSize){
	if (httpConnection->type == PROTO_HTTP){
		return recv(httpConnection->httpSocketFD, packetBuf, maxBufferSize, 0);
	}else if(type == PROTO_HTTPS){
		return SSL_read (httpConnection->sslConnection->sslHandle, packetBuf, maxBufferSize);
	}
}

