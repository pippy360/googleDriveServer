//todo: handle cases where send doesn't send all the bytes we asked it to
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

#define MAX_ACCEPTED_HTTP_PAYLOAD 200000
#define MAXDATASIZE 200000//FIXME: this should only need to be the max size of one packet !!!
#define MAX_PACKET_SIZE 1600
#define MAX_CHUNK_ARRAY_LENGTH 100
#define MAX_CHUNK_SIZE_BUFFER 100
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"

void flipBits(void* packetData, int size){
    char* b = packetData;
    int i;
    for (i = 0; i < size; ++i){
        b[i] = ~(b[i]);
    }
}

void decryptPacketData(void* packetData, int size){
    flipBits(packetData, size);
}

char *download_google_json_file(char *inputUrl){

    protocol_t type;
    char *domain, *fileUrl;
    parseUrl(inputUrl, &type, &domain, &fileUrl);
    connection *c = sslConnect( domain, "443" );

    char *result = malloc(MAX_ACCEPTED_HTTP_PAYLOAD+1);

    int outputDataLength;
    char *outputDataBuffer = result;
    parserState_t parserState;
    headerInfo_t hInfo;
    set_new_parser_state_struct(&parserState);
    set_new_header_info(&hInfo);

    char packetBuffer[MAX_PACKET_SIZE];
    //request the file
    headerInfo_t requestHeaderInfo;
    requestHeaderInfo.isRequest   = 1;
    requestHeaderInfo.requestType = GET;
    requestHeaderInfo.urlBuffer   = fileUrl;
    requestHeaderInfo.hostBuffer  = domain;
    char *accessTokenHeader = getAccessTokenHeader();
    createHTTPHeader(packetBuffer, MAX_PACKET_SIZE, &requestHeaderInfo, 
                        accessTokenHeader);
    SSL_write (c->sslHandle, packetBuffer, strlen(packetBuffer));
    //handle response
    while ( parserState.currentState != packetEnd_s ){

        int received = SSL_read (c->sslHandle, packetBuffer, MAXDATASIZE-1);
        process_data( packetBuffer, received, &parserState, outputDataBuffer, 
                        MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfo);
        outputDataBuffer += outputDataLength;
    }

    return result;
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

    set_new_parser_state_struct(parserStateBuf);
    set_new_header_info(headerInfoBuf);
    
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

void downloadFile(int client_fd, parserState_t *parserState, headerInfo_t *hInfoClientRecv, 
                    char *outputData, int outputDataLength){
    char buffer2[2000];
    char *url, *size;
    getDownloadUrlAndSize(hInfoClientRecv->urlBuffer+1, &url, &size);

    /* start downloading from google, continue until we have the whole header */
    /* send the get request to google*/
    char *accessTokenHeader = getAccessTokenHeader();
    protocol_t type;
    char *domain, *fileUrl;
    parseUrl(url, &type, &domain, &fileUrl);
    
    hInfoClientRecv->urlBuffer  = fileUrl;
    hInfoClientRecv->hostBuffer = domain;
    createHTTPHeader(buffer2, MAXDATASIZE, hInfoClientRecv, accessTokenHeader);

    connection *c = sslConnect( domain, "443" );
    SSL_write (c->sslHandle, buffer2, 2000);    

    /* get the reply and start sending stuff back to client */

    char *buffer1 = malloc(MAXDATASIZE+1);
    int received = SSL_read (c->sslHandle, buffer1, MAXDATASIZE-1);
    
    parserState_t parserStateGoogleRecv;
    set_new_parser_state_struct(&parserStateGoogleRecv);
    headerInfo_t hInfoGoogleRecv;
    set_new_header_info(&hInfoGoogleRecv);

    process_data( buffer1, received, &parserStateGoogleRecv, outputData, 
                    MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfoGoogleRecv);
    
    //check for errors        
    printf("statusCode : %d\n", hInfoGoogleRecv.statusCode);

    /* respond to client about the file */

    char moreBuffer[MAXDATASIZE];

    //hack length
    //converFromRangedToContentLength()
    if (!hInfoGoogleRecv.isRange)
    {
        if (hInfoGoogleRecv.transferType == chunked)
        {
            hInfoGoogleRecv.transferType = contentLength;
            hInfoGoogleRecv.contentLength = strtol(size, NULL, 10);
            /* sending the whole file */
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
    send(client_fd, moreBuffer, strlen(moreBuffer), 0);
    
    /* continue downloading and passing data onto the client */

    //remember to pass on any data that came with the packets that was contained in the header
    while ( parserStateGoogleRecv.currentState != packetEnd_s ){

        received = SSL_read (c->sslHandle, buffer1, MAXDATASIZE-1);
        process_data( buffer1, received, &parserStateGoogleRecv, outputData, 
            MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfoGoogleRecv);

        flipBits(outputData, outputDataLength);

        if( send(client_fd, outputData, outputDataLength, 0) == -1){
            break;
        }

        outputData[0] = '\0';
    }
    sslDisconnect(c);
}

void handle_client( int client_fd ){

    /* get the first packet from the client, 
        continue until we have the whole header and see what he wants */

    int outputDataLength;
    char *outputDataBuffer = malloc(MAXDATASIZE+1);
    parserState_t parserStateClientRecv;
    headerInfo_t hInfoClientRecv;

    getHeader(client_fd, &parserStateClientRecv, outputDataBuffer, MAXDATASIZE, 
                &outputDataLength, &hInfoClientRecv);

    /* check what the client wants by checking the URL*/
    //if ( !strncmp(hInfoClientRecv.urlBuffer, "/pull/", strlen("/pull/")) ){
        //FIXME: quick hack here
      //  hInfoClientRecv.urlBuffer = hInfoClientRecv.urlBuffer + strlen("/pull/");
        downloadFile(client_fd, &parserStateClientRecv, &hInfoClientRecv, 
                        outputDataBuffer, outputDataLength );
    //}else{
      //  printf("NOT NOT NOT starting file download !\n");
    //}
    /* if they request a file call another function */

    /* get the size and stuff from the url */


    printf("we're done apparently\n");
    close(client_fd);
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
