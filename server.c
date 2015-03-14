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
#include "net/connection.h"
#include "utils.h"
#include "google/googleAccessToken.h"
//#include "googleUpload.h"
//#include "parser.h"


#define MAX_ACCEPTED_HTTP_PAYLOAD 200000
#define MAXDATASIZE               200000//FIXME: this should only need to be the max size of one packet !!!
#define MAX_PACKET_SIZE           1600
#define MAX_CHUNK_ARRAY_LENGTH    100
#define MAX_CHUNK_SIZE_BUFFER     100
#define SERVER_LISTEN_PORT        "25001"
#define FILE_SERVER_URL           "localhost"

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

void getDownloadUrlAndSize(char *filename, char **url, char **size){
    char inputUrl[2000], *accessToken; 
    char str[MAX_ACCEPTED_HTTP_PAYLOAD];
    sprintf( inputUrl, "https://www.googleapis.com/drive/v2/files?q=title='%s'&fields"
                        "=items(downloadUrl,fileSize)", filename);

    //getAccessTokenHeader();
    headerInfo_t headerInfo;
    utils_downloadHTTPFileSimple(str, MAX_ACCEPTED_HTTP_PAYLOAD, inputUrl, 
                                    &headerInfo, accessToken);

    //todo: check if we got the file

    //todo: check it the json has what we want

    printf("Json received \n\n%s\n\n", str);
    //*url  = get_json_value("downloadUrl", str, strlen(str));
    //*size = get_json_value("fileSize", str, strlen(str));
}

//returns 0 if success, non-0 otherwise
int getHeader(Connection_t *con, parserState_t *parserStateBuf, char *outputData, 
                int outputDataMaxSize, int *outputDataLength, headerInfo_t *headerInfoBuf){

    set_new_parser_state_struct(parserStateBuf);
    set_new_header_info(headerInfoBuf);
    
    char packetBuf[MAX_PACKET_SIZE+1];
    while (!parserStateBuf->headerFullyParsed){
        int recvd = net_recv(con, packetBuf, MAX_PACKET_SIZE);
        packetBuf[ recvd ] = '\0';
        process_data(packetBuf, recvd, parserStateBuf, outputData, 
                    outputDataMaxSize, outputDataLength, packetEnd_s, headerInfoBuf);
        //todo: check for errors
    }
    return 0;
}


//TODO: what does this do ?????
void converFromRangedToContentLength(headerInfo_t *hInfo, long fileSize){
    if (!hInfo->isRange)
    {
        if (hInfo->transferType == TRANSFER_CHUNKED)
        {
            hInfo->transferType = TRANSFER_CONTENT_LENGTH;
            hInfo->contentLength = fileSize;
            /* sending the whole file */
        }else{
            printf("it's good the way it is\n\n");
        }
    }else{
        printf("google is sending us a ranged file, this is trouble "
                "because of how we deal with chunking\n");
        hInfo->transferType = TRANSFER_CONTENT_LENGTH;
        hInfo->contentLength = (hInfo->sentContentRangeEnd+1) 
                                         - hInfo->sentContentRangeStart;
    }
}

//TEST: make sure count/the return value is as expected
//FIXME: check for buffer overflow
//returns the resulting length
//targetChunkSize isn't needed 
int chunkData(const void *inputData, const int inputDataLength, void *outputBuffer){
    //calc the chunk length and get the string
    char *tempPtr = outputBuffer;
    sprintf(outputBuffer, "%x\r\n", inputDataLength);
    tempPtr += strlen(outputBuffer);
    memcpy(tempPtr, inputData, inputDataLength);
    tempPtr += inputDataLength;
    memcpy(tempPtr, "\r\n\0", strlen("\r\n")+1);
    tempPtr += strlen("\r\n");
    return tempPtr - (const char *) inputData;
}

//this will download the full header
int startFileDownload(){

//    int received;
//    char *url, *size;
//    char *domain, *fileUrl;
//    char packetBuffer[MAX_PACKET_SIZE];//reused quite a bit
//    parserState_t parserStateGoogleRecv;
//    headerInfo_t hInfoGoogleRecv;
//    protocol_t type;
//
//
//    /* request the file from google */
//
//    //connect by url here !!
//    //make a header for the file request
//    //net_send (/*connection or something*/ , packetBuffer, MAX_PACKET_SIZE);
//
//    /* get the reply and process it, handle 404's and stuff */
//
//    //TODO: change this with get the whole header !!
//    getHeader(Connection_t *con, parserState_t *parserStateBuf, char *outputData, 
//                int outputDataMaxSize, int *outputDataLength, headerInfo_t *headerInfoBuf){
//
//    printf("Reply from google:\n\n %s\n\n", packetBuffer);
//
//    set_new_parser_state_struct(&parserStateGoogleRecv);
//    set_new_header_info(&hInfoGoogleRecv);
//
//    process_data( packetBuffer, received, &parserStateGoogleRecv, outputData, 
//                    MAXDATASIZE, &outputDataLength, packetEnd_s, &hInfoGoogleRecv);
//
//    //check for errors        
//    printf("statusCode : %d\n", hInfoGoogleRecv.statusCode);
	return 0;
}

int updateFileDownload(){
	return 0;
}

int finishFileDownload(){
	return 0;
} 

void downloadDriveFile(Connection_t *clientHttpCon, parserState_t *parserState, 
                    headerInfo_t *hInfoClientRecv, char *outputData, int outputDataLength){

//    /* get the url and size of the file using the name given by the url */
//    getDownloadUrlAndSize(hInfoClientRecv->urlBuffer+1, &url, &size);    
//    printf("File found, size: %s, url: %s", size, url);
//    //
//    startFileDownload()
//    /* respond to client about the file */
//
//    //hack length
//    converFromRangedToContentLength(&hInfoGoogleRecv, strtol(size, NULL, 10));
//
//    createHTTPHeader(packetBuffer, MAX_PACKET_SIZE, &hInfoGoogleRecv, NULL);
//    net_send(clientHttpCon, packetBuffer, strlen(packetBuffer));
//    
//    /* continue downloading and passing data onto the client */
//    //send any data that might have been in the packets we fetched to get the whole header
//
//    while ( parserStateGoogleRecv.currentState != packetEnd_s ){
//
//        updateFileDownload()
//
//        flipBits(outputData, outputDataLength);
//
//        if( send_http(clientHttpCon, outputData, outputDataLength) == -1){
//            break;
//        }
//
//        outputData[0] = '\0';
//    }
//    sslDisconnect(c);
}

void handle_client( int client_fd ){

    /* get the first packet from the client, 
        continue until we have the whole header and see what he wants */

    int outputDataLength;
    char *outputDataBuffer = malloc(MAXDATASIZE+1);
    parserState_t parserStateClientRecv;
    headerInfo_t hInfoClientRecv;
    Connection_t httpCon;
    net_fileDescriptorToConnection(client_fd, &httpCon);

    getHeader(&httpCon, &parserStateClientRecv, outputDataBuffer, MAXDATASIZE, 
                &outputDataLength, &hInfoClientRecv);

    /* check what the client wants by checking the URL*/
    if ( !strncmp(hInfoClientRecv.urlBuffer, "/pull/", strlen("/pull/")) ){
        //FIXME: quick hack here
        hInfoClientRecv.urlBuffer = hInfoClientRecv.urlBuffer + strlen("/pull");
        downloadDriveFile(&httpCon, &parserStateClientRecv, &hInfoClientRecv, 
                            outputDataBuffer, outputDataLength);
    }else if ( !strncmp(hInfoClientRecv.urlBuffer, "/push/", strlen("/push/")) ){
        //printf("NOT NOT NOT starting file download !\n");

    }else{
        //404 !
    }

    printf("we're done apparently\n");
    close(client_fd);
}

int main(int argc, char *argv[])
{
    //google_init();
	AccessTokenState_t stateStruct;
	gat_init_googleAccessToken(&stateStruct);

	int sockfd = getListeningSocket(SERVER_LISTEN_PORT);
    int client_fd;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (client_fd == -1){
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            
            handle_client( client_fd );
            
            close(client_fd);
            exit(0);
        }
        close(client_fd);  // parent doesn't need this
    }


    return 0;
}
