//parses the header and chunked packets and content-length packets
//RENAME: TO HTTP PACKET PARSER
//TODO: create an option to quit once we get to a state
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHTTP.h"
#include "realtimePacketParser.h"

#define MAX_LEN 10000//FIXME: this seems really big...
#define MAX_BUFFER 1000//FIXME: do something to prevent bufferoverflows

//FIXME: THE BUFFERS SHOULDN'T BE MALLOC'D HERE
void set_new_parser_state_struct(parserState_t *parserState){
	memset(parserState, 0, sizeof(parserState_t));
	parserState->currentState 		 = getHTTPStatusLine;
	parserState->packetDataType 	 = default_empty_p;

	parserState->currentPacketPtr 	 = NULL;
	parserState->lengthBuffer 		 = malloc(MAX_BUFFER+1);
	parserState->lengthBuffer[0] 	 = '\0';
	parserState->nameBuffer 		 = malloc(MAX_BUFFER+1);
	parserState->nameBuffer[0] 		 = '\0';
	parserState->valueBuffer 		 = malloc(MAX_BUFFER+1);
	parserState->valueBuffer[0] 	 = '\0';
	parserState->statusLineBuffer	 = malloc(MAX_BUFFER+1);
	parserState->statusLineBuffer[0] = '\0';
	parserState->headerFullyParsed	 = 0;
}

void set_new_header_info(headerInfo_t *httpStats){
	memset(httpStats, 0, sizeof(headerInfo_t));

	httpStats->transferType 		 = default_empty;
	httpStats->statusStringBuffer 	 = malloc(MAX_BUFFER+1);
	httpStats->statusStringBuffer[0] = '\0';
	httpStats->urlBuffer 			 = malloc(MAX_BUFFER+1);
	httpStats->urlBuffer[0]			 = '\0';
	httpStats->hostBuffer			 = malloc(MAX_BUFFER+1);
	httpStats->hostBuffer[0]		 = '\0';
	httpStats->getContentRangeEndSet = 0;
}

void setState(parserState_t *parserState, state_t nextState){
	parserState->currentTokenPos = 0;
	parserState->currentState = nextState;
}

//0 == finished getting a valid token, 1 == still going through it, -1 == nope, failed
int token_check(parserState_t *parserState, char *outputBuffer, char *token){
	int result;
	if( parserState->currentPacketPtr[0] == token[ parserState->currentTokenPos ] ){
		if ( parserState->currentTokenPos == strlen( token ) - 1 )
			result = 0;
		else
			result = 1;
		parserState->currentTokenPos++;
	}else{
		result = -1;
		parserState->currentTokenPos = 0;
	}
	return result;
}

//this can only get hardcoded headers, to get more headers use a getHeader function on the fully downloaded header
void process_byte(char byte, parserState_t* parserState, 
				char *outputData, int outputDataMaxLength, int *outputDataLength){

	int retVal, length;
	char currByte = parserState->currentPacketPtr[0];
	switch ( parserState->currentState ){

		case getHTTPStatusLine:
			if( (retVal = token_check(parserState, NULL, "\r\n")) == 0 ){
				length = strlen( parserState->statusLineBuffer );
				parserState->statusLineBuffer[length-1] = '\0';
				setState( parserState, finishedHTTPStatusLine );
			}else{
				length = strlen( parserState->statusLineBuffer );
				parserState->statusLineBuffer[length] = currByte;
				parserState->statusLineBuffer[length+1] = '\0';
			}
			break;
		case getHeaderName:
			if( (retVal = token_check(parserState, NULL, ":")) == 0 ){
				setState( parserState, getHeaderValue );
			}else{
				//FIXME: CHECK FOR BUFFER OVERFLOW
				length = strlen( parserState->nameBuffer );
				parserState->nameBuffer[length] = currByte;
				parserState->nameBuffer[length+1] = '\0';
			}
			break;
		case getHeaderValue:
			if( (retVal = token_check(parserState, NULL, "\r\n")) == 0 ){				
				length = strlen( parserState->valueBuffer );
				parserState->valueBuffer[length-1] = '\0';
				setState( parserState, finishedHeaderValue );
			}else{
				length = strlen( parserState->valueBuffer );
				parserState->valueBuffer[length] = currByte;
				parserState->valueBuffer[length+1] = '\0';
			}
			break;
		case finishedHTTPStatusLine:
		case finishedHeaderValue:
			if( currByte == '\r' ){
				setState( parserState, finishedHeaderValue_2 );
			}else if( 1/*it's a vaild character*/ ){	
				//FIXME: this is kind of dodgy clearing the buffers here
				parserState->nameBuffer[0] = currByte;
				parserState->nameBuffer[1] = '\0';
				parserState->valueBuffer[0] = '\0';
				setState( parserState, getHeaderName );
			}else{
				//fail
			}
			break;
		case finishedHeaderValue_2:
			if( currByte == '\n' ){
				//check if it's chunked 
				parserState->headerFullyParsed = 1;
				if (parserState->packetDataType == chunked_p)
				{
					setState( parserState, getChunkLength );
				}else if (parserState->packetDataType == contentLength_p){
					setState( parserState, getContentLengthData );
				}else if (parserState->packetDataType == default_empty_p){
					setState( parserState, packetEnd_s );
				}
			}else{
				//fail
			}
			break;
		case getChunkLength:
			if( (retVal = token_check(parserState, NULL, "\r\n")) == 0 ){
				parserState->remainingLength = strtol(parserState->lengthBuffer, NULL, 16);
				if( parserState->remainingLength == 0 ){	
					setState( parserState, packetEnd_s );
				}else{
					setState( parserState, getChunkData );
				}
			}else if(retVal == 1){
			}else{
				length = strlen( parserState->lengthBuffer );
				parserState->lengthBuffer[length] = currByte;
				parserState->lengthBuffer[length+1] = '\0';
			}
			break;
		case getChunkData:
			outputData[*outputDataLength] = currByte;
			outputData[*outputDataLength+1] = '\0';
			(*outputDataLength)++;
			if ( parserState->remainingLength == 1 )
			{
				setState( parserState, finishedChunkData );
				parserState->lengthBuffer[0] = '\0';
			}
			parserState->remainingLength--;
			break;
		case finishedChunkData:
			if( (retVal = token_check(parserState, NULL, "\r\n")) == 0 ){				
				setState( parserState, getChunkLength );
			}else if(retVal != 1){
				//fail
			}
			break;
		case getContentLengthData:
			outputData[*outputDataLength] = currByte;
			outputData[*outputDataLength+1] = '\0';
			(*outputDataLength)++;
			if( parserState->remainingLength == 1 ){
				setState( parserState, packetEnd_s );
			}
			parserState->remainingLength--;
			break;
		case packetEnd_s:
			printf("hm.....hit packet end\n");
			break;
	}
}

//returns 0 if valid, non-0 otherwise
int validateHttpVersion(char *buffer, char **exitPosition){

	return 0;

	//do something like what's below

	char *ptr = buffer;
	for (; *ptr != '.'; ptr++)
		if ( *ptr < '0' || *ptr > '9' )
			printf("fail 181\n");//fail

	ptr++;
	for (; *ptr != ' '; ptr++)
		if ( *ptr < '0' || *ptr > '9' )
			printf("fail 186\n");//fail

	//check it's version
}

//fixme: this is such a crappy way of storing all the request types, fuck C for making this a pain in the ass
//TODO:ERROR CHECKING
int process_status_line(char *buffer, parserState_t* parserState, headerInfo_t* hInfo){
	int result = 0;
	char *ptr = buffer;
	//check the first few chars to see what type we have
	if( strncmp(buffer, "HTTP/", 4) == 0 ){
		hInfo->isRequest = 0;

		ptr += strlen("HTTP/1.1 ");
		char *endPtr;
		hInfo->statusCode = strtol(ptr, &endPtr, 10);
		ptr = endPtr;
		ptr++;

		int i = 0;
		for (;*ptr != '\r' && *ptr != '\0'; ptr++, i++)
			hInfo->statusStringBuffer[i] = *ptr;

		hInfo->statusStringBuffer[i] = '\0';
	}else{
		hInfo->isRequest = 1;

		if (strncmp(buffer, "GET", 3)		== 0){
			hInfo->requestType = GET;
		}else if(strncmp(buffer, "HEAD", 4)	== 0){
			hInfo->requestType = HEAD;
		}else if(strncmp(buffer, "POST", 4)	== 0){
			hInfo->requestType = POST;
		}else if(strncmp(buffer, "PUT", 3)	== 0){
			hInfo->requestType = PUT;
		}else if(strncmp(buffer, "DELETE", 6)	== 0){
			hInfo->requestType = DELETE;
		}else if(strncmp(buffer, "TRACE", 5)	== 0){
			hInfo->requestType = TRACE;
		}else if(strncmp(buffer, "OPTIONS", 7)	== 0){
			hInfo->requestType = OPTIONS;
		}else if(strncmp(buffer, "CONNECT", 7)	== 0){
			hInfo->requestType = CONNECT;
		}else if(strncmp(buffer, "PATCH", 5)	== 0){
			hInfo->requestType = PATCH;
		}else{
			//fail
			printf("fail 224\n");
		}

		ptr += strlen("GET ");

		/* get the file Url */
		int i = 0;
		for (;*ptr != ' '; ptr++, i++)
			hInfo->urlBuffer[i] = *ptr;

		hInfo->urlBuffer[i] = '\0';

		//check the http version, with function
	}

	return 0;
}


//if it's a header we're interested in, process it, otherwise just ignore
//returns -1 if invalid header, 0 otherwise 
int process_header(char *name, char *value, parserState_t* parserState, headerInfo_t* hInfo){
	int result = 0;
	
	if(strcmp("Transfer-Encoding",name) == 0 && strcmp(" chunked",value) == 0){
		//FIXME: ERROR HERE, HTTP SPEC DOESN'T SAY HOW MUCH WHITESPACE
		//FIXME: IF THERE'S A TRANSFER ENCODING WITHOUT CHUNK CAUSE ERROR
		parserState->packetDataType = chunked_p;
		hInfo->transferType			= TRANSFER_CHUNKED;

		printf("it's chunked\n");
	}else if(strcmp("Content-Length", name) == 0){
		
		parserState->packetDataType  = contentLength_p;
		hInfo->transferType		  	 = TRANSFER_CONTENT_LENGTH;
		hInfo->contentLength		 = strtol(value, NULL, 10);;
		parserState->remainingLength = strtol(value, NULL, 10);
		
		printf("it's content length : %lu\n", parserState->remainingLength);
	}else if(strcmp("Range",name) == 0){
		//"Range: bytes=106717810-114836737\r\n"\
		//TODO: error handling
		hInfo->isRange = 1;

		char *ptr = value + strlen(" bytes=");//FIXME, you guessing the number of spaces again !
		char *endPtr;
		hInfo->getContentRangeStart = strtol( ptr, &endPtr, 10);
		ptr = endPtr + 1;
		if (*ptr >= '0' && *ptr <= '9'){
			hInfo->getContentRangeEndSet = 1;
			hInfo->getContentRangeEnd = strtol( ptr, &endPtr, 10);
		}else{
			hInfo->getContentRangeEndSet = 0;
		}
	}else if(strcmp("Content-Range",name) == 0){
		//"Content-Range: bytes 106717810-114836737/114836738\r\n\r\n"
		hInfo->isRange = 1;

		char *ptr = value + strlen(" bytes ");//FIXME, you guessing the number of spaces again !
		char *endPtr;
		hInfo->sentContentRangeStart = strtol( ptr, &endPtr, 10);
		ptr = endPtr + 1;
		hInfo->sentContentRangeEnd = strtol( ptr, &endPtr, 10);
		ptr = endPtr + 1;
		hInfo->sentContentRangeFull = strtol( ptr, &endPtr, 10);
	}else if(strcmp("Host",name) == 0){
		//"Content-Range: bytes 106717810-114836737/114836738\r\n\r\n"
		memcpy( hInfo->hostBuffer, value, strlen(value) );
	}

	return 0;
}

//call this when ever we get a new packet
//TODO: ADD A FUNCTION TO EXIT ON STATE
//TODO: OFFSET REPLACE
int process_data(char *inputData, int dataLength, parserState_t* parserState, char *outputData,
				 int outputDataMaxLength, int *outputDataLength, state_t exitState, headerInfo_t* hInfo){
	state_t prevState;
	*outputDataLength = 0;
	parserState->currentPacketPtr = inputData;
	while( parserState->currentPacketPtr != inputData+dataLength ){
		prevState = parserState->currentState;
		
		process_byte( parserState->currentPacketPtr[0], parserState, outputData,
					 outputDataMaxLength, outputDataLength);

		if (parserState->currentState != prevState && parserState->currentState == finishedHeaderValue){
			process_header(parserState->nameBuffer, parserState->valueBuffer, parserState, hInfo);
		}

		if (parserState->currentState != prevState && parserState->currentState == finishedHTTPStatusLine){
			process_status_line(parserState->statusLineBuffer, parserState, hInfo);
		}

		if (parserState->currentState != prevState ){
			//printf("state: %d\n", parserState->currentState);
		}
	
		if( parserState->currentState == exitState )
			break;

		parserState->currentPacketPtr++;
	}
	//0 out the offset
}


//#define packet1 "GET /bobs/Bobs.Burgers.S02E04.HDTV.x264-LOL.mp4 HTTP/1.1\r\n"\
//	"Host: tomnomnom.me\r\n"\
//	"Connection: keep-alive\r\n"\
//	"Accept-Encoding: identity;q=1, *;q=0\r\n"\
//	"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36\r\n"\
//	"Accept: */*\r\n"\
//	"Referer: http://tomnomnom.me/bobs/Bobs.Burgers.S02E04.HDTV.x264-LOL.mp4\r\n"\
//	"Accept-Language: en-US,en;q=0.8,nl;q=0.6,ru;q=0.4\r\n"\
//	"Range: bytes=106717810-114836737\r\n"\
//	"If-Range: \"6d84502-5066f184c4756\"\r\n\r\n"
//
//#define packet2 "HTTP/1.1 206 Partial Content\r\n"\
//	"Server: nginx/1.4.6 (Ubuntu)\r\n"\
//	"Date: Sun, 14 Dec 2014 16:30:36 GMT\r\n"\
//	"Content-Type: video/mp4\r\n"\
//	"Content-Length: 0\r\n"\
//	"Connection: keep-alive\r\n"\
//	"Last-Modified: Mon, 27 Oct 2014 22:31:42 GMT\r\n"\
//	"ETag: \"6d84502-5066f184c4756\"\r\n"\
//	"Accept-Ranges: bytes\r\n"\
//	"Content-Range: bytes 106717810-114836737/114836738\r\n\r\n"
//
//
//#define packet3 "GET /bobs/Bobs.Burgers.S02E04.HDTV.x264-LOL.mp4 HTTP/1.1\r\n"\
//				"\r\n"
//
//
//int main(){
//
//	char buffer[100000];
//	int outputDataLength;
//	//ok go
//	parserState_t* parserState = get_start_state_struct();
//	headerInfo_t* hInfo = get_start_header_info();
//
//	process_data(packet2, strlen(packet2), parserState, buffer, 100000, &outputDataLength, packetEnd_s, hInfo);
//	
//	//printf("getContentRangeStart: %lu\n", 	hInfo->getContentRangeStart);
//	//printf("getContentRangeEnd: %lu\n", 	hInfo->getContentRangeEnd);
//	//printf("sentContentRangeStart: %lu\n", 	hInfo->sentContentRangeStart);
//	//printf("sentContentRangeEnd: %lu\n", 	hInfo->sentContentRangeEnd);
//	//printf("sentContentRangeFull: %lu\n", 	hInfo->sentContentRangeFull);
//	//printf("data length %d\n", outputDataLength);
//	//printf("contentLength: %lu\n", 		hInfo->contentLength);
//	//printf("transferType: %d\n", 			hInfo->transferType);
//	//printf("requestType: %d\n", 			hInfo->requestType);
//	//printf("isRequest: %d\n", 			hInfo->isRequest);
//	//printf("statusCode: %d\n", 			hInfo->statusCode);
//	//printf("statusStringBuffer: %s\n", 	hInfo->statusStringBuffer);
//	//printf("urlBuffer: %s\n", 			hInfo->urlBuffer);
//	
//
//	createHTTPHeader(buffer, 100000, hInfo, "this is an extra: header\r\n");
//
//	printf("----%s---\n", buffer );
//
//	return 0;
//}

//HTTP/1.1 200 OK
//Server: nginx/1.4.6 (Ubuntu)
//Date: Sun, 14 Dec 2014 16:30:25 GMT
//Content-Type: video/mp4
//Content-Length: 114836738
//Connection: keep-alive
//Last-Modified: Mon, 27 Oct 2014 22:31:42 GMT
//ETag: "6d84502-5066f184c4756"
//Accept-Ranges: bytes


//GET /bobs/Bobs.Burgers.S02E04.HDTV.x264-LOL.mp4 HTTP/1.1
//Host: tomnomnom.me
//Connection: keep-alive
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
//User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36
//Referer: http://tomnomnom.me/bobs/
//Accept-Encoding: gzip,deflate,sdch
//Accept-Language: en-US,en;q=0.8,nl;q=0.6,ru;q=0.4

