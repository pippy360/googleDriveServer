//todo: handle cases where send doesn't send the all the bytes we asked it to
//todo: write packet logger !!!

//TODO: who closes the connection ?

//TODO: learn error handling

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

#define MAXDATASIZE 2000
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define BACKLOG 10

char file_request_headers[] = " HTTP/1.1\r\nHost: www.googleapis.com\r\nContent-length: 0\r\n\r\n"
void flipBits(void* packetData, int size){
    char* b = packetData;
    int i;
    for (i = 0; i < size; ++i){
        b[i] = ~(b[i]);
    }
}

//TODO: can handle all packets, with/without headers, chunked or not
void decryptPacketData(void* packetData, int size){
    flipBits(packetData, size);
}

char* strstrn(char* const haystack, const char* needle, const int haystackSize){
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

char* head_request(char* filePath){
    char* output = malloc( strlen("HEAD ") + strlen(filePath) + strlen(file_request_headers) + 1 );
    output[0] = '\0';
    strcat(output, "HEAD ");
    strcat(output, filePath);
    strcat(output, file_request_headers);
}

//handles 'moved permanently'/'404'/http->https
//given a url it will "follow" it to the file's direct download link
char* get_file_url(char* proto, char* host, char* fileUrl){
    //connect to the server
    if ( strcmp(proto, "https") == 0 ){
        sslConnect( host, "443" );
    }else if( strcmp(proto, "http") == 0 ){
        set_up_tcp_connection( host, "80" );
    }else{
        perror("bad proto");
    }
    //and head it for the file
    //handle the head
    //if 200
        //return the filw 
    //if moved {temp or perm}
        //return get_file_url
    //if 404
        //return null
    //else 
        //return null
}

char* get_file_url(char* inputUrl){
    //connect to the server
}

//reads the next packet from the client and returns the encrypted data 
//returns null when it's finished
char* get_file_piece_by_piece(char* filePath,  ){

}

long get_content_length(char* buf, int bufSize){
    
    char* ptr;
    if ((ptr = strstrn( buf, "Content-Length: ", bufSize )) != NULL ){
        return strtol( ptr + strlen("Content-Length: "), '\0', 10 );
    }
    return -1;
}

int strip_packet(char* const buf, const int bufSize, void** header, int* headerSize,
                    void** data, int* dataSize){
    if ((*data = strstrn( buf, "\r\n\r\n", bufSize )) != NULL ){
        *data += 4;
        *header = buf;
        *headerSize = *data - *header;
        *dataSize = bufSize - *headerSize;
        return 0;
    }
    return -1;
}

void handle_file_request(char* clientGetRequest, int clientGetRequestLength, int client_fd){

    //process the clients get request
    char    tempBuf[MAXDATASIZE];

    //get the file the client requested from the packet
    char*   filePath;
    if(get_requsted_file_string(clientGetRequest, clientGetRequestLength, &filePath) == -1)
        printf("failed to extract the file from clients get request\n");

    printf("get request: %s\n", filePath);

    char*   proxiedRequestData;
    int     proxiedRequestDataLength;
    proxyGetRequest(clientGetRequest, clientGetRequestLength, FILE_SERVER_URL, &proxiedRequestData, &proxiedRequestDataLength);

    //set up a connection to fileServer
    int fileServer_fd = set_up_tcp_connection(FILE_SERVER_URL, "80");
    if( send(fileServer_fd, proxiedRequestData, proxiedRequestDataLength, 0) == -1 )
        printf("get request to fileServer failed to send\n");

    //get the first packet from the fileServer
    int tempByteCount = recv(fileServer_fd, tempBuf, MAXDATASIZE-1, 0);

    long fileSize = get_content_length(tempBuf, tempByteCount);//get the file length
    //seperate the header and the data
    int headerSize, dataSize;
    void *data, *header;
    strip_packet( tempBuf, tempByteCount, &header, &headerSize,
                    &data, &dataSize);

    //decrypt it
    decryptFirstPacketData(data, dataSize);
    
    //send the first response packet to the client
    if( send(client_fd, tempBuf, tempByteCount, 0) == -1 )
        printf("send failed\n");

    //then handle the remaining bytes
    long remaining = fileSize - dataSize;
    for ( ;remaining != 0; remaining -= dataSize){
        
        dataSize = recv(fileServer_fd, tempBuf, MAXDATASIZE-1, 0);
        decryptPacketData(data, dataSize);

        if( send(client_fd, tempBuf, dataSize, 0) == -1 ){
            printf("failed to proxy file piece to client \n");
            //logPacket( )
        }
    }

    printf("file sent: %s : %lu B \n", filePath, fileSize);
    //TODO: FREE EVERYTHING !!!
    close(fileServer_fd);
}

int is_get( char *packet, int packetLength ){
    return (packetLength > 2 && strstrn(packet, "GET", 3) != NULL);
}

void handle_client( int client_fd )
{
    //see what the client wants
    char* firstClientPacket = malloc( MAXDATASIZE );
    int firstClientPacketLength = recv(client_fd, firstClientPacket, MAXDATASIZE-1, 0);

    if( is_get(firstClientPacket, firstClientPacketLength) ){    
        handle_file_request(firstClientPacket, firstClientPacketLength, client_fd);
    }else{
        printf("non get packet received\n");
    }    
}

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

int get_listening_socket(char* port){

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    int rv;
    struct sigaction sa;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    return sockfd;
}

int get_requsted_file_string(char* buf, int inputPacketLength, char** outputStr){

    char* ptr;
    if ( (ptr = strstrn(buf, " HTTP", inputPacketLength)) != NULL ){
        int length = ptr - (buf + strlen("GET "));
        *outputStr = malloc( length + 1 );
        (*outputStr)[ length ] = '\0';
        memcpy( *outputStr, buf + strlen("GET "), length);
        return 0;
    }
    return -1;
}

//return: 0 if success, -1 if there is no host, -2 if packet is broken
//if return == -1 then *outputPacket points to the inputPacket
//if return == -2 then the packet should be thrown away and an error should be sent to the host
int changeHost(char *inputPacket, int inputPacketLength, char *newHost, 
                    char **outputPacket, int* outputPacketLength){

    char *ptr, *newPtr;
    if ( (ptr = strstrn(inputPacket, "Host: ", inputPacketLength)) != NULL )
    {
        int offset = (ptr + strlen("Host: ")) - inputPacket;        
        if ( (newPtr = strstrn(ptr + strlen("Host: "), "\r\n", inputPacketLength - offset)) == NULL ){
            perror("bad packet, couldn't find \\r\\n");
            return -2;
        }
        
        int restOfPacketLength = inputPacketLength - (newPtr - inputPacket);
        *outputPacketLength = offset + strlen(newHost) + restOfPacketLength;
        *outputPacket = malloc( *outputPacketLength );

        memcpy(*outputPacket, inputPacket, offset);
        memcpy(*outputPacket + offset, newHost, strlen(newHost));
        memcpy(*outputPacket + offset + strlen(newHost), newPtr, restOfPacketLength );

        return 0;
    }else{
        *outputPacket = inputPacket;
        *outputPacketLength = inputPacketLength;
        return -1;
    }
}

//return: 0 if success, -1 if it's not a get, -2 if packet is broken
int changeFileRequest(char *buf, int inputPacketLength, char *newFilePath, 
                        char **outputPacket, int* outputPacketLength){

    if ( is_get(buf, inputPacketLength) )
    {
        int offset = strlen("GET ");
        char *newPtr;
        if ( (newPtr = strstrn(buf, " HTTP/", inputPacketLength)) == NULL ){
            perror("bad packet, couldn't find HTTP/");
            return -2;
        }
        *outputPacketLength = offset + strlen(newFilePath) + inputPacketLength - (newPtr - buf);
        
        *outputPacket = malloc(*outputPacketLength);
        memcpy(*outputPacket, buf, offset);
        memcpy(*outputPacket + offset, newFilePath, strlen(newFilePath));
        memcpy(*outputPacket + offset + strlen(newFilePath), newPtr, inputPacketLength - (newPtr - buf));
        return 0;
    }else{
        printf("changeFileRequest called on bad packet\n");
        return -1;
    }
}

int proxyGetRequest(void *inputPacket, const int inputPacketLength, 
                    const char *fileServer, void **output, int *outputLength){
    
    //FIXME: WASTED BUFFER
    char *tempPacket;
    char *finalPacket;
    int tempPacketLength, finalPacketLength;
    //copy the old Packet
    char* buffer = malloc( MAXDATASIZE );
    memcpy(buffer, inputPacket, inputPacketLength);

    //change the host
    if( changeHost(buffer, inputPacketLength, FILE_SERVER_URL, &tempPacket, &tempPacketLength) == -1 ){
        printf("Get packet did not contain Host...weird...\n");
    }

    //remove
    //change the file request
    char *requestedFile;
    get_requsted_file_string(tempPacket, tempPacketLength, &requestedFile);
    changeFileRequest(tempPacket, tempPacketLength, requestedFile, &finalPacket, &finalPacketLength);

    *output = finalPacket;
    *outputLength = finalPacketLength;
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

//
// opensll stuff
//

// Establish a connection using an SSL layer
connection *sslConnect (char* host, char* port)
{
  connection *c;

  c = malloc (sizeof (connection));
  c->sslHandle = NULL;
  c->sslContext = NULL;

  c->socket = set_up_tcp_connection(host, port);
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

//
// /-- opensll stuff
//


int main(void)
{

    return 0;
}