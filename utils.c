#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net/networking.h"
#include "net/connection.h"

//TODO: PARSE URL

char* strstrn(char* const haystack, const char* needle, const int haystackSize){
    if (haystackSize < strlen(needle))
        return NULL;

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

//just like utils_parseUrl but it gives you the protocol in type protocol_t
//returns 0 if valid url, -1 otherwise
int utils_parseUrl_proto(const char *inputUrl, const int inputUrlLength, protocol_t *type, char **domain, 
                        int *domainLength, char **fileUrl, int *fileUrlLength){

    int protoSize = ptr - inputUrl;

    utils_parseUrl(const char *inputUrl, char **protocol, int *protocolLength, char **domain, 
                    int *domainLength, char **fileUrl, int *fileUrlLength  )

    if( strncmp( inputUrl, "https", (protoSize > strlen("https"))? protoSize : strlen("https") ) == 0 ){
        *type = PROTO_HTTPS;
    }else if( strncmp( inputUrl, "http", (protoSize > strlen("http"))? protoSize : strlen("http")) == 0 ){
        *type = PROTO_HTTP;
    }else{
        printf("ERROR: BAD PROTOCOL\n");
        *type = PROTO_HTTP;
    }
}

//FIXME:
//TODO: enum for http/https/ftp
//TODO: should work with lower case too and there shouldn't be so much hardcoded stuff
//this will break a lot, "example.com" breaks it
//returns 0 if valid url, -1 otherwise
int utils_parseUrl(const char *inputUrl, const int inputUrlLength, char **protocol, 
                    int *protocolLength, char **domain, int *domainLength, char **fileUrl, 
                    int *fileUrlLength  ){

    //break at '://' and check if it matches http/https
    char *ptr = strstrn(inputUrl, "://", strlen(inputUrl));

    ptr += strlen("://");
    int remaining = strlen(inputUrl) - (ptr - inputUrl);

    //FIXME: THIS IS WAY TOO HARD TO READ ! HAVE A BETTER WAY OF GETTING STRING SIZE !
    //get the url (until we hit "/" )
    char* newPtr = strstrn(ptr, "/", remaining);
    int domainLen = (newPtr - ptr);
    *domain = malloc( domainLen + 1 );
    (*domain)[ domainLen ] = '\0';
    
    memcpy( *domain, ptr, domainLen );

    remaining = strlen(inputUrl) - (newPtr - inputUrl);

    *fileUrl = malloc( remaining + 1 );
    (*fileUrl)[ remaining ] = '\0';
    memcpy( *fileUrl, newPtr, remaining);
}

void utils_getAccessTokenHeader(){

//    char *accessToken = getAccessToken();
//    char headerStub[]  = "Authorization: Bearer ";
//
//    char* tokenHeader = malloc( strlen(headerStub) + strlen(accessToken) + 1 + 2 );
//    sprintf( tokenHeader, "%s%s%s", headerStub, accessToken, "\r\n");
//
//    return tokenHeader;
}

void getConnectionStructByUrl(const char *inputUrl, Connection_t *con){
    protocol_t type;
    char *fileUrl;

    //FIXME: REDO PARSE URL
    utils_parseUrl(inputUrl, &type, domain, &fileUrl);//TODO: this needs to be fixed when you fix parse url
    if (type == PROTO_HTTP){
        con->type = PROTO_HTTP;
    }else if(type == PROTO_HTTPS){
        con->type = PROTO_HTTPS;
    }
}

//returns an active tcp connection to the url 
void utils_connectByUrl(const char *inputUrl, Connection_t *con){
    char *domain;
    net_setupNewConnection(con);
    getConnectionStructByUrl(inputUrl, con);
    
    utils_parseUrl_proto(const char *inputUrl, int inputUrlLength, protocol_t *type, char **domain, 
                        int *domainLength, char **fileUrl, int *fileUrlLength)

    net_connect(con, domain);
}

void utils_createHTTPGetHeaderFromUrl(char *inputUrl, int inputUrlLength, char *output, int maxOutputLen, 
                                    headerInfo_t *hInfo, char *extraHeaders){
    set_new_header_info(hInfo);
    
    protocol_t type;
    char *domain, *fileUrl;
    
    utils_parseUrl_proto(const char *inputUrl, protocol_t *type, char **domain, 
                        int *domainLength, char **fileUrl, int *fileUrlLength)
    
    hInfo->isRequest   = 1;
    hInfo->requestType = GET;
    hInfo->urlBuffer   = fileUrl;
    hInfo->hostBuffer  = domain;
    createHTTPHeader(output, maxOutputLen, hInfo, extraHeaders);
}

//returns the amount of data downloaded (excluding the header)
int utils_downloadHTTPFileSimple(char *outputBuffer, const int outputMaxLength, 
                        char *inputUrl, headerInfo_t *hInfo, char *extraHeaders){

    Connection_t httpConnection;
    utils_connectByUrl(inputUrl, &httpConnection);

    parserState_t parserState;
    set_new_parser_state_struct(&parserState);
    set_new_header_info(hInfo);

    char packetBuffer[MAX_PACKET_SIZE];
    //request the file
    headerInfo_t requestHeaderInfo;
    createHTTPGetHeaderFromUrl(inputUrl, packetBuffer, MAX_PACKET_SIZE, &requestHeaderInfo, extraHeaders);

    printf("Packet to GET json: \n\n%s\n\n", packetBuffer);

    send_http(&httpConnection, packetBuffer, strlen(packetBuffer));
    //handle response

    char *tempPtr = outputBuffer;
    while ( parserState.currentState != packetEnd_s ){

        int received = recv_http(&httpConnection, packetBuffer, MAXDATASIZE-1);
        int outputDataLength;
        process_data( packetBuffer, received, &parserState, tempPtr, 
                        MAXDATASIZE, &outputDataLength, packetEnd_s, hInfo);
        tempPtr += outputDataLength;
    }
}