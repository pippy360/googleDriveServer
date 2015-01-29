#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    char *accessToken = getAccessToken();
    char headerStub[]  = "Authorization: Bearer ";

    char* tokenHeader = malloc( strlen(headerStub) + strlen(accessToken) + 1 + 2 );
    sprintf( tokenHeader, "%s%s%s", headerStub, accessToken, "\r\n");

    return tokenHeader;
}

//the domain stuff here is hacky
void getConnectionByUrl(const char *inputUrl, Connection_t *httpConnection){
    protocol_t type;
    char *fileUrl;

    //FIXME: REDO PARSE URL
    parseUrl(inputUrl, &type, domain, &fileUrl);
    if (type == PROTO_HTTP){
        httpConnection->type = PROTO_HTTP;
    }else if(type == PROTO_HTTPS){
        httpConnection->type = PROTO_HTTPS;
    }
}

