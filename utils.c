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

char* getAccessTokenHeader(){

//    char *accessToken = getAccessToken();
//    char headerStub[]  = "Authorization: Bearer ";
//
//    char* tokenHeader = malloc( strlen(headerStub) + strlen(accessToken) + 1 + 2 );
//    sprintf( tokenHeader, "%s%s%s", headerStub, accessToken, "\r\n");
//
//    return tokenHeader;
    return NULL;
}

//the domain stuff here is hacky
void getConnectionByUrl(const char *inputUrl, Connection_t *httpConnection){
//    protocol_t type;
//    char *fileUrl;
//
//    //FIXME: REDO PARSE URL
//    parseUrl(inputUrl, &type, domain, &fileUrl);
//    if (type == PROTO_HTTP){
//        httpConnection->type = PROTO_HTTP;
//    }else if(type == PROTO_HTTPS){
//        httpConnection->type = PROTO_HTTPS;
//    }
}

void connectByUrl(char *inputUrl, Connection_t *httpConnection){
//    set_new_httpConnection(httpConnection);
//    char *domain;//TODO: this needs to be fixed when you fix parse url
//    printf("the input url is : %s\n", inputUrl);
//    getConnectionByUrl(inputUrl, httpConnection, &domain);
//    printf("the domain was detected as : %s\n", domain);
//    connect_http(httpConnection, domain);
}

//void createHTTPGetHeaderFromUrl(char *inputUrl, char *output, int maxOutputLen, 
//                                    headerInfo_t *hInfo, char *extraHeaders){
//  set_new_header_info(hInfo);
//    
//    protocol_t type;
//    char *domain, *fileUrl;
//    parseUrl(inputUrl, &type, &domain, &fileUrl);
//    
//    hInfo->isRequest   = 1;
//    hInfo->requestType = GET;
//    hInfo->urlBuffer   = fileUrl;
//    hInfo->hostBuffer  = domain;
//    createHTTPHeader(output, maxOutputLen, hInfo, extraHeaders);
//}