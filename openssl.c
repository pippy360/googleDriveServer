#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// Simple structure to keep track of the handle, and
// of what needs to be freed later.
typedef struct {
    int socket;
    SSL *sslHandle;
    SSL_CTX *sslContext;
} connection;

// For this example, we'll be testing on openssl.org
#define SERVER  "www.googleapis.com"
#define PORT "443"
#define MAXDATASIZE 1500

char access_token[] = "ya29.uACtRPa-kqUXiIyHXXBYkZ6FRGQD8OhNQmdV_PnBP5SowKkgzM244x7LBi3IwogGtTo7LeOUWGUz3w";

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* strstrn(char* const haystack, const char* needle, const int haystackSize){
    if ( strlen(needle) > haystackSize )
      return NULL;

    int i;
    for (i = 0; i < (haystackSize - (strlen(needle) - 1)); i++){
        int j;
        for (j = 0; j < strlen(needle); j++)
            if(needle[j] != haystack[i+j])
                break;

        if (j == strlen(needle))
            return haystack + i;
    }
    return NULL;
}

int set_up_tcp_connection(const char* hostname, const char* port){

    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}

// Establish a connection using an SSL layer
connection *sslConnect (char* server, char* port)
{
  connection *c;

  c = malloc (sizeof (connection));
  c->sslHandle = NULL;
  c->sslContext = NULL;

  c->socket = set_up_tcp_connection(server, port);
  if (c->socket)
    {
      // Register the error strings for libcrypto & libssl
      SSL_load_error_strings ();
      // Register the available ciphers and digests
      SSL_library_init ();

      // New context saying we are a client, and using SSL 2 or 3
      c->sslContext = SSL_CTX_new (SSLv23_client_method ());
      if (c->sslContext == NULL)
        ERR_print_errors_fp (stderr);

      // Create an SSL struct for the connection
      c->sslHandle = SSL_new (c->sslContext);
      if (c->sslHandle == NULL)
        ERR_print_errors_fp (stderr);

      // Connect the SSL struct to our connection
      if (!SSL_set_fd (c->sslHandle, c->socket))
        ERR_print_errors_fp (stderr);

      // Initiate SSL handshake
      if (SSL_connect (c->sslHandle) != 1)
        ERR_print_errors_fp (stderr);
    }
  else
    {
      perror ("Connect failed");
    }

  return c;
}

// Disconnect & free connection struct
void sslDisconnect (connection *c)
{
  if (c->socket)
    close (c->socket);
  if (c->sslHandle)
    {
      SSL_shutdown (c->sslHandle);
      SSL_free (c->sslHandle);
    }
  if (c->sslContext)
    SSL_CTX_free (c->sslContext);

  free (c);
}

// Very basic main: we send GET / and print the response.
int main (int argc, char **argv)
{
  char buffer[ MAXDATASIZE ];
  char text[] = "GET /drive/v2/files?access_token=ya29.uAC9kjLZtZRHpMUwgRraRYcbLQnzBM960O3wYEvpnG2ze_bFRj6uKPz_1_xPBpmsJoU5_wbWQsNJ8Q HTTP/1.1\r\nHost: www.googleapis.com\r\nContent-length: 0\r\n\r\n";
  int fileSize;
  char* ptr;

  connection *c = sslConnect (SERVER, PORT);

  SSL_write (c->sslHandle, text, strlen (text));
  int received = SSL_read (c->sslHandle, buffer, MAXDATASIZE);
  buffer[received] = '\0';
  
  while((received = SSL_read (c->sslHandle, buffer, MAXDATASIZE)) > 0){
    buffer[received] = '\0';
    if( strstrn(buffer, "\r\n0\r\n\r\n", received) != NULL ){
      printf("%.*s", (int) (received-strlen("\r\n0\r\n\r\n")), buffer);
      break;
    }
    printf("%s", buffer);
  }

  sslDisconnect (c);
  return 0;
}