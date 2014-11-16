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
#define MAX_CHUNK_SIZE_BUFFER 100
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define GET_HEADER_FORMAT "GET %s HTTP/1.1\r\nHost: %s\r\n%s"\
                    ""\
                    "Content-length: %d\r\n\r\n%s"

typedef struct {
    int   state;//0-download finished, 1-downloading the header, 2-downloading data
    int   isChunked;//chuncked or content-length 
    long  amount_of_file_downloaded;
    long  fileSize;//not set if chuncked
    long  current_chunk_size;
    long  current_chunk_remaining;
    char  current_chunk_size_buffer[ MAX_CHUNK_SIZE_BUFFER ];//a buff
    int   chunk_state;//0-waiting for length, 1-waiting for chunk start, 2-active download
    char* directFileUrl;
    char* headers;
    int   headers_length;
    int   headers_buffer_size;//the actual allocated size of the header buffer
    char* current_data_buffer;
    int   http_code;
} fileDownload;

typedef struct
{
    connection *currConnection;//chuncked or content-length 
    char* firstPacket;
    char* current_packet;
    int   current_packet_length;
    char* directFileUrl;
    long  amount_of_data_downloaded;
    int   current_packet_no;//0 = first packet
} packet_organiser;

typedef struct {
    long chunkSize;
    long dataInPacket;
    char* data;
} chunk;

fileDownload* create_fileDownload(){
    fileDownload *fd = malloc( sizeof(fileDownload) );
    fd->state = 1;
    fd->headers = NULL;
    fd->chunkState = 1;//0-waiting for length, 1-waiting for chunk start, 2-active download
    fd->current_chunk_size_buffer[0] = '\0';
}

void free_fileDownload(fileDownload *fd){
    //TODO:
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
        return strtol( ptr + strlen("Content-Length: "), NULL, 10 );
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
    return (int)strtol( buffer, NULL, 10 );
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
        printf("val of chunk size    : %d\n", (int)c->chunkSize );
        arr[arrayLength] = c;
        if ( c->dataInPacket != c->chunkSize )
            break;        
        arrayLength++;
    }
    *chunkc = arrayLength;
    *chunkv = arr;
}

//REMOVE HEADER BEFORE PASSING PACKET IN
//todo: this may not be needed, just get all the chunks in a packet and check the last one for 0 length
int is_final_chunked_packet(char *packet, int packetLength){
    chunk **chunkv;
    int  chunkc, i;
    get_all_chunks_in_packet(packet, packetLength, &chunkv, &chunkc);
    for (i = 0; i < chunkc; ++i)
        if (chunkv[i]->chunkSize == 0)
            return 1;

    return 0;
}

void fill_packet_organiser(packet_organiser *po, char *packet, int packetSize, 
                        char *inputUrl, connection *c )
{
    po->currConnection  = c;//chuncked or content-length 
    po->firstPacket     = packet;
    po->current_packet  = packet;
    po->current_packet_length = packetSize;
    po->directFileUrl   = inputUrl;
    po->amount_of_data_downloaded += packetSize;
    po->current_packet_no = 0;
}

//return the first packet of a file download
//TODO: this function is too long
//ONLY HANDLES PACKET ORGANISATION
int start_file_download(char* inputUrl, char* addedHeaders, packet_organiser **po)
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

    *po = malloc( sizeof( packet_organiser ) );
    fill_packet_organiser( *po, response, received, inputUrl, c );

    return 0;//TODO: why ?
}

//remember this is a file download, not just a get next packet
//returns NULL when the file download is done
//download is done when 0 chunk or when size complete
int get_next_packet_file_download(packet_organiser *po)
{
    //get the open socket from the fileDownload
    //if it's closed fix it somehow 
    //free the previous packet
    //if it's chunked check set the chunk state
    char* buffer = malloc(MAXDATASIZE);
    int received = SSL_read (po->currConnection->sslHandle, buffer, MAXDATASIZE-1);
    buffer[received] = '\0';
    po->current_packet = buffer;
}


//TODO: this can be cleaned up
void add_to_header_buffer(fileDownload *fd, char *data, int dataLength){
    //set the headers_length
    if ( fd->headers == NULL )
    {
        fd->headers = malloc( dataLength + 1 );
        fd->headers_buffer_size = dataLength + 1;
        memcpy( fd->headers, data, dataLength );
        fd->headers[dataLength] = '\0';
        fd->headers_length = dataLength;
    
    }else{
        fd->headers = realloc(fd->headers, fd->headers_length + dataLength + 1);
        fd->headers_buffer_size = fd->headers_length + dataLength + 1;
        memcpy( (fd->headers + fd->headers_length), data, dataLength );
        fd->headers_length = fd->headers_length + dataLength;  
        fd->headers[fd->headers_length] = '\0';
    }
}

int process_packet_header(packet_organiser *po, fileDownload *fd){
    
    //check for end in this packet
    char *ptr;
    int offset;
    if( (ptr = strstrn(po->current_packet, "\r\n\r\n", po->current_packet_length)) != NULL ){
        ptr += 4;
        fd->state = 2;
        add_to_header_buffer(fd, po->current_packet, (ptr - po->current_packet));
        offset = (ptr - po->current_packet);
    }else{
        add_to_header_buffer(fd, po->current_packet, po->current_packet_length);
    }

    //FIXME: THIS SHOULD BE ALLOWED TO FAIL !
    fd->http_code = get_http_response_code( fd->headers, fd->headers_length );

    if( 0/*the packet contains content-lenght*/ ){
        //todo: actually implement this 
    
    }else if ( strstrn(fd->headers, "Transfer-Encoding: chunked", fd->headers_length) != NULL ){
        fd->isChunked = 1;
    }

    return offset;
}

//TODO:
void add_to_data_buffer(fileDownload *fd, char *data, int dataLength){
    if ( fd->headers == NULL )
    {
        fd->headers = malloc( dataLength + 1 );
        fd->headers_buffer_size = dataLength + 1;
        memcpy( fd->headers, data, dataLength );
        fd->headers[dataLength] = '\0';
        fd->headers_length = dataLength;
    
    }else{
        fd->headers = realloc(fd->headers, fd->headers_length + dataLength + 1);
        fd->headers_buffer_size = fd->headers_length + dataLength + 1;
        memcpy( (fd->headers + fd->headers_length), data, dataLength );
        fd->headers_length = fd->headers_length + dataLength;  
        fd->headers[fd->headers_length] = '\0';
    }
}

//FIXME: this is really messy and poorly tested
void process_packet_data_chunks(packet_organiser *po, fileDownload *fd, int offset){
    //get all the chunks in a packet
    //all set the state and load the size buffer/data buffer
    //set the packet stats
    //LOOK OUT FOR 0 BYTE CHUNK
    char *data = po->current_packet + offset;
    char *pos = data;

    while(1){

        if( fd->chunkState == 1 ){
            char *ptr;
            //check we can read the whole length
            if ( (ptr = strstrn(data, "\r\n", po->current_packet_length - offset)) != NULL )
            {
                pos = ptr + strlen("\r\n");
                fd->current_chunk_size_buffer[0] = '\0'; //wipe the length buffer
                fd->current_chunk_size = strtol( fd->current_chunk_size_buffer, NULL, 10);
                fd->chunkState = 2;
                //FIXME: bug here, if the zero chunk spans over two packets then the last packet will be ignored
                if ( fd->current_chunk_size == 0 ){
                    fd->state = 0;
                    //set everything else that should be set
                    return;
                }
            }else{
                int dataSize = po->current_packet_length - (data - po->current_packet);
                sprintf( fd->current_chunk_size_buffer, "%s%.*s", fd->current_chunk_size_buffer, dataSize, data;
            }
        }

        if( fd->chunkState == 2 || fd->chunkState == 3 ){
            int remainingPacketSize =
            if ( /*the data goes to the next packet*/ ){
                //add_to_data_buffer(fd, data, dataLength);
                fd->chunkState = 3;
                break;
            }else{
                //pump everythign 
                printf("DATA: %.*s",  )//add_to_data_buffer(fd, data, dataLength);
                fd->chunkState = 1;
            }
        }
    }
}

//TODO: MAKE SURE IT CAN HANDLE MULTIPLE CHUNKS IN ONE PACKET
void process_packet_data(packet_organiser *po, fileDownload *fd, int offset){
    char* data = po->current_packet + offset;
    printf("processing the data, offset...data...: %s\n", po->current_packet + offset);
    //set if the download if finished or not
    if ( fd->isChunked ){
        process_packet_data_chunks(po, fd, offset);
    }else{
        /*handle non chunked data*/
    }
}

//remember this is per packet
void process_packet(packet_organiser *po, fileDownload *fd){

    int offset = 0;
    if (fd->state == 1)
    {
        offset = process_packet_header(po, fd);
    }

    if (fd->state == 2)
    {
        /* TODO SOME WAY OF PASSING ON THE OFFSET */
        process_packet_data(po, fd, offset);
    }
}

int main(void)
{
    google_init();
    
    //fetch a file,
    int type;
    char *domain, *fileUrl;
    char inputUrl[] = "https://www.googleapis.com/drive/v2/files/";
    
    //create the access token
    //TODO: write get access token header
    char *accessTokenHeader = getAccessTokenHeader();

    packet_organiser *po;
    start_file_download(inputUrl, accessTokenHeader, &po);

    //todo: !!
    fileDownload *fd = create_fileDownload();
    process_packet( po, fd );

    //while ( fd->state != 0 )
    //{
    //    get_next_packet_file_download(po);
    //    printf("packet: \n\n%s\n", po->current_packet );
    //}

    
    return 0;
}