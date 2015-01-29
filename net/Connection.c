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

#include "../httpProcessing/commonHTTP.h"
#include "networking.h"
#include "../parser.h"

#include "httpConnection.h"

/*
 *
 *
 * this page contains http/https independent connect/send/recv functions
 *
 *
 */

//pass in a working+connected http (not https) fileDescriptor
void getHttpConnectionByFileDescriptor(int fd, Connection_t *httpCon){
	set_new_httpConnection(httpCon);
	httpCon->type = PROTO_HTTP;
	httpCon->httpSocketFD = fd;
}

void connectByUrl(char *inputUrl, Connection_t *httpConnection){
	set_new_httpConnection(httpConnection);
	char *domain;//TODO: this needs to be fixed when you fix parse url
	printf("the input url is : %s\n", inputUrl);
	getHttpConnectionByUrl(inputUrl, httpConnection, &domain);
	printf("the domain was detected as : %s\n", domain);
	connect_http(httpConnection, domain);
}

void set_new_httpConnection(Connection_t *httpConnection){
	memset(httpConnection, 0, sizeof(Connection_t));
}

void freeHttpConnection(Connection_t *input){
	//todo
	//call the free sslConnection rather than freeing it here
}


int connect_http(Connection_t *httpConnection, char *domain){
	if (httpConnection->type == PROTO_HTTP){
		httpConnection->httpSocketFD = set_up_tcp_connection(domain, "80");
	}else if(httpConnection->type == PROTO_HTTPS){
		httpConnection->sslConnection = sslConnect(domain, "443");
	}
	//todo, return value !
}

int close_http(Connection_t *httpConnection, char *domain){
	if (httpConnection->type == PROTO_HTTP){
		//todo
	}else if(httpConnection->type == PROTO_HTTPS){
		//todo
	}
}

int send_http(Connection_t *httpConnection, char *packetBuf, int dataSize){
	if (httpConnection->type == PROTO_HTTP){
		return send(httpConnection->httpSocketFD, packetBuf, dataSize, 0);
	}else if(httpConnection->type == PROTO_HTTPS){
		return SSL_write (httpConnection->sslConnection->sslHandle, packetBuf, dataSize);
	}
}

int recv_http(Connection_t *httpConnection, char *packetBuf, int maxBufferSize){
	if (httpConnection->type == PROTO_HTTP){
		return recv(httpConnection->httpSocketFD, packetBuf, maxBufferSize, 0);
	}else if(httpConnection->type == PROTO_HTTPS){
		return SSL_read (httpConnection->sslConnection->sslHandle, packetBuf, maxBufferSize);
	}
}

