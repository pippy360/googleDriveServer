//todo: handle cases where send doesn't send the all the bytes we asked it to
//todo: write packet logger !!!
//TODO: who closes the connection ?
//TODO: learn error handling
//"Note that the major and minor numbers MUST be treated as"
//"separate integers and that each MAY be incremented higher than a single digit."
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
    connection *currConnection;//chuncked or content-length 
    int   isChunked;//chuncked or content-length 
    char* directFileUrl;
    char* firstPacket;
    char* currentPacket;
    int   currentPacketNo;//0 = first packet
    long  amountDownloaded;
    long  fileSize;//not set if chuncked
    int   state;
    long  currChunkSize;
    long  currChunkRemaining;
} fileDownload;


void free_fileDownload(fileDownload *fd){
    free( fd->directFileUrl );
    free( fd->firstPacket );
    free( fd->currentPacket );
    free( fd );
}

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

int is_get( char *packet, int packetLength ){
    return (packetLength > 2 && strstrn(packet, "GET", 3) != NULL);
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

//NOTE: this probably won't be needed
// a space is automatically set between the id and new value, so don't inlude it
int set_header_value(char *inputPacket, int inputPacketLength, char *identifier, char *newValue, 
                        char **outputPacket, int* outputPacketLength)
{
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

int get_http_response_code(char* packet, int packetLength){
    char *ptr;
    for (ptr = packet; *ptr != ' '; ptr++)
        ;
    
    char buffer[4];
    int i;
    for ( i=0, ptr += 1; *ptr != ' ' ; ptr++, i++)
        buffer[i] = *ptr;

    buffer[3] = '\0';
    return (int)strtol( buffer, '\0', 10 );
}

int is_chunked(char* packet, int packetLength){
    return strstrn(packet, "Transfer-Encoding: chunked", packetLength) != NULL;
}

void get_packet_data(packet, packetSize, isChunked, datav, datac, datavSize){

}

//return the first packet of a file download
//it returns the html error code, returns 200 if OK
int start_file_download(char* inputUrl, char* addedHeaders, fileDownload** fd)
{
    int type;
    char *domain, *fileUrl;
    parseUrl(inputUrl, &type, &domain, &fileUrl);

    //get first packet
    char *getData = getRequestData( fileUrl, domain, "", addedHeaders );
    
    //connect
    connection *c;
    if (type == 1){
        c = sslConnect( domain, "443" );
    }else{
        printf("non https protocol used....hm.....\n");
    }
    //send it
    SSL_write (c->sslHandle, getData, strlen (getData));

    char* buffer = malloc(MAXDATASIZE);
    int received = SSL_read (c->sslHandle, buffer, MAXDATASIZE-1);
    buffer[received] = '\0';

    int code = get_http_response_code( buffer, received );

    if (code >= 300 || code < 200)
    {
        printf("get: \n\n%s\n\n", getData );
        printf("packet: \n\n%s\n\n", buffer );
        return code;
    }

    int isChunked = is_chunked( buffer, received );

    //get_packet_data(packet, packetSize, isChunked, datav, datac, datavSize);
    
    *fd = malloc( sizeof( fileDownload ) );
    (*fd)->isChunked          = isChunked; 
    (*fd)->directFileUrl      = inputUrl;
    (*fd)->firstPacket        = buffer;
    (*fd)->currentPacket      = buffer;
    (*fd)->currentPacketNo    = 0;
    (*fd)->amountDownloaded   = 0;//TODO: check the amount of data downloaded
    (*fd)->fileSize           = 0;//check if this can be set or not
    (*fd)->currConnection     = c;
    (*fd)->state              = 1;//TODO: enum this
    //SET THE CHUNKING STUFF AND THE STATE
    return code;
}

//remember this is a file download, not just a get next packet
//returns null when the file download is done
//download is done when 0 chunk or when size complete
int get_next_packet_file_download(fileDownload *fd)
{
    //get the open socket from the filedownload
    //if it's closed fix it somehow 
    //free the previous packet
    char* buffer = malloc(MAXDATASIZE);
    int received = SSL_read (fd->currConnection->sslHandle, buffer, MAXDATASIZE-1);
    buffer[received] = '\0';
    fd->currentPacket = buffer;
}

int main(void)
{
    google_init();
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

    fileDownload *fd;
    int code = start_file_download(inputUrl, addedHeader, &fd);
    printf("code: %d\n", code);
    printf("firstPacket: %s\n", fd->firstPacket);

    while ( fd->state != 0 )
    {
        get_next_packet_file_download(fd);
        printf("packet: \n\n%s\n", fd->currentPacket );
    }

    //now create the get request
    return 0;
}