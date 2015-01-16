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
#include "httpProcessing/commonHTTP.h"//TODO: parser states
#include "httpProcessing/realtimePacketParser.h"//TODO: parser states
#include "httpProcessing/createHTTPHeader.h"
#include "utils.h"
#include "googleAccessToken.h"
#include "googleUpload.h"
#include "parser.h"

#define MAXDATASIZE 200000//FIXME: this should only need to be the max size of one packet !!!
#define MAX_PACKET_SIZE 1600
#define MAX_CHUNK_ARRAY_LENGTH 100
#define MAX_CHUNK_SIZE_BUFFER 100
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define GET_HEADER_FORMAT "GET %s HTTP/1.1\r\nHost: %s\r\n%s"\
                    ""\
                    "Content-length: %d\r\n\r\n%s"

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

char *getRequestData(char *fileUrl, char *domain, char *content, char *addedHeaders ){
    char *output = malloc( strlen(GET_HEADER_FORMAT) + strlen(fileUrl) + strlen(domain) + strlen(content) 
        + strlen(addedHeaders) );
    sprintf( output, GET_HEADER_FORMAT, fileUrl, domain, addedHeaders, (int)strlen(content), content );

    return output;
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
    
    parserState_t parserState;
    set_new_parser_state_struct(&parserState);
    headerInfo_t hInfo;
    set_new_header_info(&hInfo);

    process_data( po->current_packet, po->current_packet_length, &parserState, 
                  outputDataBuffer, MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfo);
    
    outputDataBuffer += outputDataLength;

    while ( parserState.currentState != packetEnd_s ){
        get_next_packet_file_download(po);
        parserState.currentPacketPtr = po->current_packet;
        process_data( po->current_packet, po->current_packet_length, &parserState, 
                      outputDataBuffer, MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfo);
        outputDataBuffer += outputDataLength;
    }

    return start;
}

void getDownloadUrlAndSize(char *file, char **url, char **size){
    char inputUrl[2000];
    sprintf( inputUrl, "https://www.googleapis.com/drive/v2/files?q=title='%s'&fields=items(downloadUrl,fileSize)", file);
    char *str = download_google_json_file( inputUrl );
    *url  = get_json_value("downloadUrl", str, strlen(str));
    *size = get_json_value("fileSize", str, strlen(str));
}

//returns 0 if success, non-0 otherwise
int getHeader(int client_fd, parserState_t *parserStateBuf, char *outputData, 
                int outputDataMaxSize, int *outputDataLength, headerInfo_t *headerInfoBuf){

    //set the parser state to start state
    set_new_parser_state_struct(parserStateBuf);
    set_new_header_info(headerInfoBuf);
    //set the header info  to start state
    char packetBuf[MAX_PACKET_SIZE+1];
    while (!parserStateBuf->headerFullyParsed){
        int recvd = recv(client_fd, packetBuf, MAX_PACKET_SIZE, 0);
        packetBuf[ recvd ] = '\0';
        process_data(packetBuf, recvd, parserStateBuf, outputData, 
                    outputDataMaxSize, outputDataLength, packetEnd_s, headerInfoBuf);
        //todo: check for errors
    }
    return 0;
}

void handle_client( int client_fd ){

    /* get the first packet from the client, 
        continue until we have the whole header and see what he wants */

    char buffer2[2000];
    int outputDataLength;
    char *outputDataBuffer = malloc(MAXDATASIZE+1);
    parserState_t parserStateClientRecv;
    headerInfo_t hInfoClientRecv;

    getHeader(client_fd, &parserStateClientRecv, buffer2, MAXDATASIZE, 
                &outputDataLength, &hInfoClientRecv);

    /* check what the client wants by checking the URL*/

    /* if they request a file call another function */

    /* get the size and stuff from the url */

    char *url, *size;
    getDownloadUrlAndSize(hInfoClientRecv.urlBuffer+1, &url, &size);
    printf("now print size: %s\n", size);
    printf("now print url: %s\n", url);

    /* start downloading from google, continue until we have the whole header */
    /* send the get request to google*/
    char *accessTokenHeader = getAccessTokenHeader();

    printf("Range data, isRange %d, getStart %lu getEnd %lu \n", hInfoClientRecv.isRange, 
        hInfoClientRecv.getContentRangeStart, hInfoClientRecv.getContentRangeEnd);
    
    protocol_t type;
    char *domain, *fileUrl;
    parseUrl(url, &type, &domain, &fileUrl);
    
    printf("is it set ? %d\n", hInfoClientRecv.getContentRangeEndSet);
    hInfoClientRecv.urlBuffer  = fileUrl;
    printf("%s\n", hInfoClientRecv.urlBuffer);
    hInfoClientRecv.hostBuffer = domain;
    printf("%s\n", hInfoClientRecv.hostBuffer);
    createHTTPHeader(buffer2, MAXDATASIZE, &hInfoClientRecv, accessTokenHeader);

    printf("lets look at the google header \n\n %s\n\n", buffer2);

    connection *c = sslConnect( domain, "443" );

    SSL_write (c->sslHandle, buffer2, 2000);    
    printf("packet sent to google \n\n%s\n", buffer2 );

    /* get the reply and start sending stuff back to client */

    char *buffer1 = malloc(MAXDATASIZE+1);
    int received = SSL_read (c->sslHandle, buffer1, MAXDATASIZE-1);
    printf("the packet from google \n\n%s\n", buffer1);

    parserState_t parserStateGoogleRecv;
    set_new_parser_state_struct(&parserStateGoogleRecv);
    headerInfo_t hInfoGoogleRecv;
    set_new_header_info(&hInfoGoogleRecv);
    printf("fail here \n");
    printf("%d\n",received);
    process_data( buffer1, received, &parserStateGoogleRecv, outputDataBuffer, 
                    MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfoGoogleRecv);
    
    printf("fail here \n");
        
    printf("statusCode : %d\n", hInfoGoogleRecv.statusCode);

    /* respond to client about the file */

    char moreBuffer[MAXDATASIZE];
    
    if (!hInfoGoogleRecv.isRange)
    {
        if (hInfoGoogleRecv.transferType == chunked)
        {
            hInfoGoogleRecv.transferType = contentLength;
            hInfoGoogleRecv.contentLength = strtol(size, NULL, 10);
            printf("sending the whole file\n");
            /* code */
        }else{
            printf("it's good the way it is\n\n");
        }
    }else{
        printf("google is sending us a ranged file, this is trouble because of how we deal with chunking\n");
        hInfoGoogleRecv.transferType = contentLength;
        hInfoGoogleRecv.contentLength = (hInfoGoogleRecv.sentContentRangeEnd+1) 
                                         - hInfoGoogleRecv.sentContentRangeStart;
    }

    createHTTPHeader(moreBuffer, MAXDATASIZE, &hInfoGoogleRecv, NULL);

    printf("packet Sent to client response: \n\n%s\n\n", moreBuffer);

    send(client_fd, moreBuffer, strlen(moreBuffer), 0);
    
    /* continue downloading and passing data onto the client */

    //remember to pass on any data that came with the packets that was contained in the header
    while ( parserStateGoogleRecv.currentState != packetEnd_s ){

        received = SSL_read (c->sslHandle, buffer1, MAXDATASIZE-1);
        process_data( buffer1, received, &parserStateGoogleRecv, outputDataBuffer, 
            MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfoGoogleRecv);

        flipBits(outputDataBuffer, outputDataLength);

        if( send(client_fd, outputDataBuffer, outputDataLength, 0) == -1){
            break;
        }

        outputDataBuffer[0] = '\0';
    }
    printf("we're done apparently\n");
    close(client_fd);
    sslDisconnect(c);
    //check how the whole process went...

}

int main(int argc, char *argv[])
{
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
