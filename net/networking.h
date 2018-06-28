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

// Simple structure to keep track of the handle, and
// of what needs to be freed later.
typedef enum { 
	TCP_DIRECT,//non-ssl connection
	TCP_SSL
} TCP_PROTOCOL_T;

typedef struct {
    int socket;
    SSL *sslHandle;
    SSL_CTX *sslContext;
} sslConnection;

void *get_in_addr(struct sockaddr *sa);

int getPort(int fd);

int getListeningSocket(const char* port);

//returns a valid file descriptor, or -1 if error
int setUpTcpConnection(const char* hostname, const char* port);

//
// opensll stuff
//

// Establish a connection using an SSL layer
//returns 0 if success, -1 otherwise
int sslConnect (const char* host, const char* port, sslConnection *c);

// Disconnect & free connection struct
void sslDisconnect (sslConnection *c);
