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


#define MAXDATASIZE 2000
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define BACKLOG 10

char file_request_headers[] = " HTTP/1.1\r\nHost: www.googleapis.com\r\nContent-length: 0\r\n\r\n";

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



int main(void)
{

    return 0;
}