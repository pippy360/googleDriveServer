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
#include "networking.h"
#include "utils.h"
#include "google.h"
#include "parser.h"

#define MAXDATASIZE 2000
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define GET_HEADER_FORMAT "GET %s HTTP/1.1\r\nHost: %s\r\n%s"\
                    ""\
                    "Content-length: %d\r\n\r\n%s"

typedef struct {
    char* filename;
    char* directFileUrl;
    int   downloadType;//chuncked or content-length
} fileDownload;


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
    //proxyGetRequest(clientGetRequest, clientGetRequestLength, FILE_SERVER_URL, &proxiedRequestData, &proxiedRequestDataLength);

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
    flipBits(data, dataSize);
    
    //send the first response packet to the client
    if( send(client_fd, tempBuf, tempByteCount, 0) == -1 )
        printf("send failed\n");

    //then handle the remaining bytes
    long remaining = fileSize - dataSize;
    for ( ;remaining != 0; remaining -= dataSize){
        
        dataSize = recv(fileServer_fd, tempBuf, MAXDATASIZE-1, 0);
        flipBits(data, dataSize);

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

// a space is automatically set between the id and new value, so don't inlude it
int set_header_value(char *inputPacket, int inputPacketLength, char *identifier, char *newValue, 
                        char **outputPacket, int* outputPacketLength)
{
//TODO:
}

/*

"POST %s HTTP/1.1\r\nHost: %s\r\n"\
                    "Content-Type: application/x-www-form-urlencoded\r\n"\
                    "Content-length: %d%s\r\n\r\n%s"

*/


char *getRequestData(char *fileUrl, char *domain, char *content, char *addedHeaders ){
    char *output = malloc( strlen(GET_HEADER_FORMAT) + strlen(fileUrl) + strlen(domain) + strlen(content) 
        + strlen(addedHeaders) );
    sprintf( output, GET_HEADER_FORMAT, fileUrl, domain, addedHeaders, (int)strlen(content), content );

    return output;    
}

//return the first packet of a file download
//it returns the html error code, returns 200 if OK
int start_file_download(char* inputUrl, char* addedHeaders, fileDownload** fd, 
                            char** firstPacket, int* firstPacketLength)
{
    int type;
    char *domain, *fileUrl;
    parseUrl(inputUrl, &type, &domain, &fileUrl);

    //get first packet
    char *getData = getRequestData( fileUrl, domain, "", addedHeaders );
    printf("%s\n", getData);

    //connect
    connection *c;
    if (type == 1){
        c = sslConnect( domain, "443" );
    }else{
        printf("non https protocol used....hm.....\n");
    }

    //send it
    SSL_write (c->sslHandle, getData, strlen (getData));

    char buffer[MAXDATASIZE+1];
    int received = SSL_read (c->sslHandle, buffer, MAXDATASIZE-1);

    buffer[received] = '\0';
    printf("%s\n", buffer);

    received = SSL_read (c->sslHandle, buffer, MAXDATASIZE-1);

    buffer[received] = '\0';
    printf("%s\n", buffer);

    //set the struct
    //set the datails and return the error code
}

int get_next_packet_file_download()
{

}

int main(void)
{
    //fetch a file,
    int type;
    char *domain, *fileUrl;
    char inputUrl[] = "https://www.googleapis.com/drive/v2/files/";
    parseUrl(inputUrl, &type, &domain, &fileUrl);

    char* accessToken = getAccessToken();
    char headerStub[] = "Authorization: Bearer ";
    char addedHeader[strlen(headerStub) + strlen(accessToken) + 1 + 2];
    addedHeader[0] = '\0';
    strcat(addedHeader, headerStub);
    strcat(addedHeader, accessToken);
    strcat(addedHeader, "\r\n");

    start_file_download(inputUrl, addedHeader, NULL, NULL, NULL);
    //now create the get request
    return 0;
}