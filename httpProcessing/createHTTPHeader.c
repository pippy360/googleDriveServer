#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHTTP.h"
#include "realtimePacketParser.h"

#define REQUEST_HEADERS \
	"Connection: keep-alive\r\n"\
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
	"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36\r\n"\
	"Connection: keep-alive\r\n"\
	"Accept-Encoding: identity;q=1, *;q=0\r\n"\
	"Accept-Language: en-US,en;q=0.8,nl;q=0.6,ru;q=0.4\r\n"
	
#define RESPONSE_HEADERS \
	"Server: nginx/1.4.6 (Ubuntu)\r\n"\
	"Connection: keep-alive\r\n"\
	"Accept-Ranges: bytes\r\n"


char *toRequestStr(httpRequestTypes_t requestType){
	switch( requestType ){	
		case REQUEST_GET: 		return "GET";
		case REQUEST_HEAD: 		return "HEAD";
		case REQUEST_POST: 		return "POST";
		case REQUEST_PUT: 		return "PUT";
		case REQUEST_DELETE: 	return "DELETE";
		case REQUEST_TRACE: 	return "TRACE";
		case REQUEST_OPTIONS: 	return "OPTIONS";
		case REQUEST_CONNECT: 	return "CONNECT";
		case REQUEST_PATCH: 	return "PATCH";
	}
	return NULL;//fail
}

//it's important that this works even if the content length isn't set
//FIXME: MAKE SURE THE BUFFER IS BIG ENOUGH
//returns the amount of data written to *output
int createHTTPHeader(char *output, const int maxOutputLen, const HTTPHeaderState_t *hInfo, const char *extraHeaders){

	/* status line first */
	if ( hInfo->isRequest ){
		sprintf(output, "%s %s %s\r\n", toRequestStr( hInfo->requestType ), hInfo->urlBuffer, HTTP_VERSION );
		sprintf(output+strlen(output), "Host: %s\r\n", hInfo->hostBuffer);
	}else{
		sprintf(output, "%s %d %s\r\n", HTTP_VERSION, hInfo->statusCode, hInfo->statusStringBuffer );
	}


	/* now the request/response specific headers*/
	if ( hInfo->isRequest ){
		int l = strlen(output);
		strcpy(output+l, REQUEST_HEADERS);
		l += strlen(REQUEST_HEADERS);
		//"Range: bytes=106717810-114836737\r\n"
		if( hInfo->isRange ){
			if (hInfo->getEndRangeSet){
				sprintf( output+l, "Range: bytes=%lu-%lu\r\n", hInfo->getContentRangeStart, 
						hInfo->getContentRangeEnd);
			}else{				
				sprintf( output+l, "Range: bytes=%lu-\r\n", hInfo->getContentRangeStart);
			}
		}
		l = strlen(output);
		//"Host: doc-0o-as-docs.googleusercontent.com\r\n"
	}else{
		int l = strlen(output);
		strcpy(output+l, RESPONSE_HEADERS);
		l += strlen(RESPONSE_HEADERS);
		//"Content-Range: bytes 106717810-114836737/114836738\r\n\r\n"
		if( hInfo->isRange ){
			sprintf( output+l, "Content-Range: bytes %lu-%lu/%lu\r\n", hInfo->sentContentRangeStart, 
					hInfo->sentContentRangeEnd, hInfo->sentContentRangeFull);
		}		
	}

	//now the added headers
	if ( extraHeaders )
		strcat(output, extraHeaders);

	/* now the common headers*/
	int l = strlen(output);
	if( hInfo->transferType == TRANSFER_CONTENT_LENGTH ){
		sprintf( output+l, "Content-length: %lu\r\n", hInfo->contentLength);
	}else if( hInfo->transferType == TRANSFER_CHUNKED ){
		sprintf( output+l, "Transfer-Encoding: chunked\r\n");
	}

	strcat(output, "\r\n");
	return strlen(output);
}


