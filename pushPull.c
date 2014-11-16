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
#define MAX_CHUNK_ARRAY_LENGTH 100
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

typedef struct {
    long chunkSize;
    long dataInPacket;
    char* data;
} chunk;

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

int strip_packet(char* const buf, const int bufSize, char** header, int* headerSize,
                    char** data, int* dataSize){
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

//offset should point to the start of the chunk
void get_next_chunk(char *chunkStart, int packetLength, char** endPtr, chunk **resultChunk){
    char *tempPtr;
    *resultChunk = malloc( sizeof(chunk) );
    (*resultChunk)->chunkSize = strtol(chunkStart, &tempPtr, 16);
    tempPtr += strlen("\r\n");
    (*resultChunk)->data = tempPtr;
    (*resultChunk)->dataInPacket = packetLength - (tempPtr - chunkStart);
    *endPtr = tempPtr + (*resultChunk)->dataInPacket;
}

//remove the http headers before passing a packet into this
void get_all_chunks_in_packet(char* packet, int packetLength, chunk*** chunkv, int* chunkc){
    int offset = 0;
    int arrayLength = 1;
    char *ptr = packet;
    char *endPtr;
    chunk **arr = malloc( MAX_CHUNK_ARRAY_LENGTH*sizeof( chunk* ) );
    while(1){
        chunk *c;
        get_next_chunk(ptr, packetLength, &endPtr, &c);
        printf("ptr: %s\n---ptr/\n", ptr);
        ptr = endPtr;
        printf("val of data in packet: %d\n", (int)c->dataInPacket );
        arr[arrayLength] = c;
        if ( c->dataInPacket != c->chunkSize )
            break;        
        arrayLength++;
    }
    *chunkc = arrayLength;
    *chunkv = arr;
}

//REMOVE HEADER BEFORE PASSING PACKET IN
//todo: this may not be need, just get all the chunks in a packet and check the last one for 0 length
int is_final_chunked_packet(char *packet, int packetLength){
    chunk **chunkv;
    int  chunkc, i;
    get_all_chunks_in_packet(packet, packetLength, &chunkv, &chunkc);
    for (i = 0; i < chunkc; ++i)
        if (chunkv[i]->chunkSize == 0)
            return 1;

    return 0;
}

void fill_fileDownload(fileDownload *fd, char *packet, int packetSize, char *inputUrl ){
    int isChunked = is_chunked( packet, packetSize );
    fd->isChunked            = isChunked;
    fd->directFileUrl        = inputUrl;
    fd->firstPacket          = packet;
    fd->currentPacket        = packet;
    fd->currentPacketNo      = 0;

    //strip the packet
    char *header, *data;
    int headerSize, dataSize;
    strip_packet(packet, packetSize, &header, &headerSize, &data, &dataSize);

    printf("packet --\n%s---packet/\n", packet );

    if ( isChunked )
    {
        chunk **chunkv;
        int chunkc;
        get_all_chunks_in_packet(data, dataSize, &chunkv, &chunkc);

        fd->currChunkSize = chunkv[chunkc-1]->chunkSize;
        fd->currChunkRemaining = (chunkv[chunkc-1]->chunkSize) - (chunkv[chunkc-1]->dataInPacket);
        if ((chunkv[chunkc-1]->chunkSize) == 0)
            fd->state = 0;//finished download
        else
            fd->state = 1;//still downloading
        long final = 0, i;
        for (i = 0; i < chunkc; ++i)
            final += chunkv[i]->dataInPacket;

        fd->amountDownloaded = final;
    }else{
        long len = get_content_length(packet, packetSize);
        fd->fileSize = len;
        if (len == dataSize)
            fd->state = 0;//finished download
        else
            fd->state = 1;//still downloading

        fd->amountDownloaded = dataSize;
    }
}

//return the first packet of a file download
//it returns the html error code, returns 200 if OK
//TODO: this function is too long
int start_file_download(char* inputUrl, char* addedHeaders, fileDownload** fd)
{
    int type;
    char *domain, *fileUrl;
    parseUrl(inputUrl, &type, &domain, &fileUrl);
    //get first packet
    char *getData = getRequestData( fileUrl, domain, "", addedHeaders );
    
    //connect
    connection *c;
    if (type == 1)
        c = sslConnect( domain, "443" );
    else
        printf("non https protocol used....hm.....\n");

    SSL_write (c->sslHandle, getData, strlen (getData));

    //get reply
    char* response = malloc(MAXDATASIZE);
    int received = SSL_read (c->sslHandle, response, MAXDATASIZE-1);
    response[received] = '\0';

    int code = get_http_response_code( response, received );
    if (code >= 300 || code < 200)
        return code;

    *fd = malloc( sizeof( fileDownload ) );
    fill_fileDownload( *fd, response, received, inputUrl );


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