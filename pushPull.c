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
#include "net/networking.h"
#include "net/realtimePacketParser.h"//TODO: parser states
#include "utils.h"
#include "googleAccessToken.h"
#include "googleUpload.h"
#include "parser.h"

#define MAXDATASIZE 200000//FIXME: this should only need to be the max size of one packet !!!
#define MAX_CHUNK_ARRAY_LENGTH 100
#define MAX_CHUNK_SIZE_BUFFER 100
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define GET_HEADER_FORMAT "GET %s HTTP/1.1\r\nHost: %s\r\n%s"\
                    ""\
                    "Content-length: %d\r\n\r\n%s"

#define temp_server_header "HTTP/1.1 200 OK\r\nContent-Length: %s\r\n\r\n"

typedef struct
{
    connection *currConnection;//chuncked or content-length 
    char* firstPacket;
    char* current_packet;
    int   current_packet_length;
    char* directFileUrl;
    long  amount_of_data_downloaded;
    int   current_packet_no;//0 = first packet
    protocol_t type;
} packet_organiser;

typedef struct {
    long chunkSize;
    long dataInPacket;
    char* data;
} chunk;

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

void fill_packet_organiser(packet_organiser *po, char *packet, int packetSize, 
                        char *inputUrl, connection *c, protocol_t type )
{
    po->currConnection  = c;//chuncked or content-length 
    po->firstPacket     = packet;
    po->current_packet  = packet;
    po->current_packet_length = packetSize;
    po->directFileUrl   = inputUrl;
    po->amount_of_data_downloaded += packetSize;
    po->current_packet_no = 0;
    po->type = type;
}

//return the first packet of a file download
//TODO: this function is too long
//ONLY HANDLES PACKET ORGANISATION
int start_file_download(char* inputUrl, char* addedHeaders, packet_organiser **po)
{
    protocol_t type;
    char *domain, *fileUrl;
    parseUrl(inputUrl, &type, &domain, &fileUrl);
    //get first packet
    char *getData = getRequestData( fileUrl, domain, "", addedHeaders );

    //connect
    connection *c;
    if (type == https)
        c = sslConnect( domain, "443" );
    else if(type == http)
        c = set_up_tcp_connection_struct( domain, "80" );
    else
        ;//fail
 
    if (type == https)
        SSL_write (c->sslHandle, getData, strlen(getData));
    else if(type == http)
        send(c->socket, getData, strlen(getData), 0);

    //printf("getdata--%s--\n", getData);

    //get reply
    char* response = malloc(MAXDATASIZE);
    
    int received;
    if (type == https)
        received = SSL_read (c->sslHandle, response, MAXDATASIZE-1);
    else if(type == http)
        received = recv(c->socket, response, MAXDATASIZE-1, 0);

    response[received] = '\0';

    *po = malloc( sizeof( packet_organiser ) );
    fill_packet_organiser( *po, response, received, inputUrl, c, type );

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
    //TODO: you can save on the malloc by just overwriting the previous current packet buffer
    int received;
    if (po->type == https){
        received = SSL_read (po->currConnection->sslHandle, po->current_packet, MAXDATASIZE-1);
    }else{
        received = recv(po->currConnection->socket, po->current_packet, MAXDATASIZE-1, 0);
    }

    po->current_packet[received] = '\0';
    char *ptr = po->current_packet;
    po->current_packet_length = received;
}

char *download_google_json_file(char *inputUrl){

    char *accessTokenHeader = getAccessTokenHeader();
    accessTokenHeader = getAccessTokenHeader();
    packet_organiser *po;
    start_file_download(inputUrl, accessTokenHeader, &po);
    
    int outputDataLength;
    char *start = malloc(MAXDATASIZE+1);
    char *outputDataBuffer = start;
    
    parserState_t *parserState = get_start_state_struct();
    headerInfo_t *hInfo   = get_start_header_info();

    process_data( po->current_packet, po->current_packet_length, parserState, 
                  outputDataBuffer, MAXDATASIZE, &outputDataLength, packetEnd, hInfo);
    
    outputDataBuffer += outputDataLength;

    while ( parserState->currentState != packetEnd ){
        get_next_packet_file_download(po);
        parserState->currentPacketPtr = po->current_packet;
        process_data( po->current_packet, po->current_packet_length, parserState, 
                      outputDataBuffer, MAXDATASIZE, &outputDataLength, packetEnd, hInfo);
        outputDataBuffer += outputDataLength;
    }

    return start;
}

void getDownloadUrlAndSize(char *file, char **url, char **size){
    char inputUrl[2000];
    sprintf( inputUrl, "https://www.googleapis.com/drive/v2/files?q=title='%s'&fields=items(downloadUrl,fileSize)", file);
    //char inputUrl[5000] = "https://www.googleapis.com/drive/v2/files?q=title='upload8.jpg'&fields=items(downloadUrl,fileSize)";
    //char inputUrl[] = "https://doc-14-as-docs.googleusercontent.com/docs/securesc/he9unpj0rh27t96v09626udpvmum8vcs/in4i4ddgph7ut164onm5k83v87ek74e8/1417190400000/13058876669334088843/00377112155790015384/0B7_KKsaOads4aE5wTUZRTmFEa28?e=download&gd=true";
    char *str = download_google_json_file( inputUrl );
    *url  = get_json_value("downloadUrl", str, strlen(str));
    *size = get_json_value("fileSize", str, strlen(str));
}

void handle_client( int client_fd ){


    /* get the first packet from the client, continue until we have the whole header, discard any data*/
    //TODO: write this properly
    char buffer[2000];
    int recvd = recv( client_fd, buffer, 2000, 0);
    buffer[ recvd ] = '\0';

    int i = 4;
    for (; buffer[i] != ' ' ; i++)
        ;
    buffer[i] = '\0';

    printf("fileurl: --%s--\n", buffer + 5);


    char *url, *size;
    getDownloadUrlAndSize(buffer + 5, &url, &size);
    printf("now printing url and size:\n");
    printf("%s\n", size);
    printf("%s\n", url);

    /* start downloading from google, continue until we have the whole header */
    char *accessTokenHeader = getAccessTokenHeader();
    packet_organiser *po;
    start_file_download(url, accessTokenHeader, &po);
    
    int outputDataLength;
    char *outputDataBuffer = malloc(MAXDATASIZE+1);
    parserState_t *parserState = get_start_state_struct();
    headerInfo_t *hInfo   = get_start_header_info();

    process_data( po->current_packet, po->current_packet_length, parserState, 
                  outputDataBuffer, MAXDATASIZE, &outputDataLength, packetEnd, hInfo);

    //respond to client
    //TODO: seperate the downloader

    char moreBuffer[MAXDATASIZE];
    sprintf(moreBuffer, temp_server_header, size);
    send(client_fd, moreBuffer, strlen(moreBuffer), 0);
    
    /* continue downloading and passing data onto the client */

    //remember to pass on any data that came with the packets that contained the header
    do{

        while ( parserState->currentState != packetEnd ){
            get_next_packet_file_download(po);
            parserState->currentPacketPtr = po->current_packet;
            process_data( po->current_packet, po->current_packet_length, parserState, 
                        outputDataBuffer, MAXDATASIZE, &outputDataLength, packetEnd, hInfo);

            //hm......
            flipBits(outputDataBuffer, outputDataLength);
            if( send(client_fd, outputDataBuffer, outputDataLength, 0) == -1){
                break;
            }

            outputDataBuffer[0] = '\0';
        }

    }while(0);


    printf("we're done apparently\n");
    close(client_fd);
    sslDisconnect(po->currConnection);
    //check how the whole process went...

}

int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("error, bad argc\n");
        return -1;
    }

    google_init();

    int sockfd = get_listening_socket(SERVER_LISTEN_PORT);
    int client_fd;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            
            handle_client( client_fd );
            
            close(client_fd);
           //recv from client, now get a file from google

            exit(0);
        }
        close(client_fd);  // parent doesn't need this
    }


    return 0;
}

    /*
    //time for pull
    google_init();
    
    char fileBuffer[2000];

    int counter;
    FILE *fp;
    fp = fopen("big_text.txt","r");
    
    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    
    //fetch a file,
    int type;
    char *domain, *fileUrl;
    char *accessTokenHeader = getAccessTokenHeader();
    char buffer[20000];

    int noChange, packetSize, dataSent;
    char fileData[] = "firstline tea;flkjadsf;lkjasfd;lkjsaasf;lkkjas;lfj;sakdjf ;lsakdjf;lksadjf;lasdkjf;lsadkjf;lsadkjf;lsadkjf;lasdkjf;lasdkjf;lsadkjf;sdakjf;sadlkjf;lsadkjfawjept2 dsf aslkdfj a;sdflk asdf a kf;ldsa l;asd j;lfjals dfen asdlfkjs;adlkfj sadf dsalkjf ;ldsa k d -----------------------------------end";
    int length = strlen( fileData );
    //get a file and load it 
    googleUploadStruct *stateStruct = getGoogleUploadStruct_struct();
    char *metadata = "{\n  \"title\": \"My File2\"\n}\n";

    connection *c;
    c = sslConnect( "www.googleapis.com", "443" );


    while( stateStruct->parserState != finished ){ 
        int read = fread( fileBuffer, 1, 300, fp);
        packetSize = getNextUploadPacket(fileBuffer, read, &dataSent, filesize, metadata, accessTokenHeader, 
                                        stateStruct, buffer, 2000, &noChange);
        SSL_write(c->sslHandle, buffer, packetSize);
        if ( dataSent != read )
        {
            printf("BAD FILE SEND\n");
        }
    }

    printf("sent:\n");

    int received = SSL_read(c->sslHandle, buffer, MAXDATASIZE-1);
    printf("%s\n", buffer);

    fclose(fp);
    return 0;
    */