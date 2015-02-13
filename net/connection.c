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
#include "connection.h"

#include "../parser.h"


/*
 *
 *
 * this page contains http/https independent connect/send/recv functions
 *
 *
 */

//pass in a working+connected non-ssl (not https) fileDescriptor
void net_fileDescriptorToConnection(int fd, Connection_t *con){
	set_new_con(con);
	con->type = TCP_DIRECT;
	con->httpSocketFD = fd;
}

void net_setupNewConnection(Connection_t *con){
	memset(con, 0, sizeof(Connection_t));
}

void net_freeConnection(Connection_t *input){
	//todo
	//call the free sslConnection rather than freeing it here
}


int net_connect(Connection_t *con, char *domain){
	if (con->type == TCP_DIRECT){
		con->httpSocketFD = setUpTcpConnection(domain, "80");
	}else if(con->type == TCP_SSL){
		if(sslConnect(domain, "443", con.sslConnection) != 0)
			perror("sslConnect");
	}
	//todo, return value !
}

int net_close(Connection_t *con, char *domain){
	if (con->type == TCP_DIRECT){
		//todo
	}else if(con->type == TCP_SSL){
		//todo
		//call ssl dissconnectandfree
	}
}

int net_send(Connection_t *con, char *packetBuf, int dataSize){
	if (con->type == TCP_DIRECT){
		return send(con->httpSocketFD, packetBuf, dataSize, 0);
	}else if(con->type == TCP_SSL){
		return SSL_write (con->sslConnection->sslHandle, packetBuf, dataSize);
	}
}

int net_recv(Connection_t *con, char *packetBuf, int maxBufferSize){
	if (con->type == TCP_DIRECT){
		return recv(con->httpSocketFD, packetBuf, maxBufferSize, 0);
	}else if(con->type == TCP_SSL){
		return SSL_read(con->sslConnection->sslHandle, packetBuf, maxBufferSize);
	}
}

