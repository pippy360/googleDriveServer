//parses the header and chunked packets and content-length packets
//RENAME: TO HTTP PACKET PARSER
//TODO: create an option to quit once we get to a state
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHTTP.h"
#include "realtimePacketParser.h"

#define MAX_LEN 10000//FIXME: this seems really big...
#define MAX_BUFFER 6000//FIXME: do something to prevent bufferoverflows

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
	httpStats->getEndRangeSet		 = 0;
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
		case BAD_PACKET:
			printf("ERROR BAD PACKET\n");
	}
}

//returns 0 if valid, non-0 otherwise
int validateHttpVersion(char *buffer, char **exitPosition){

	return 0;

	//do something like what's below
/*
	char *ptr = buffer;
	for (; *ptr != '.'; ptr++)
		if ( *ptr < '0' || *ptr > '9' )
			printf("fail 181\n");//fail

	ptr++;
	for (; *ptr != ' '; ptr++)
		if ( *ptr < '0' || *ptr > '9' )
			printf("fail 186\n");//fail
*/
	//check it's version
}

//fixme: this is such a crappy way of storing all the request types, fuck C for making this a pain in the ass
//TODO:ERROR CHECKING
int process_status_line(char *buffer, parserState_t* parserState, headerInfo_t* hInfo){
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
			hInfo->requestType = REQUEST_GET;
		}else if(strncmp(buffer, "HEAD", 4)	== 0){
			hInfo->requestType = REQUEST_HEAD;
		}else if(strncmp(buffer, "POST", 4)	== 0){
			hInfo->requestType = REQUEST_POST;
		}else if(strncmp(buffer, "PUT", 3)	== 0){
			hInfo->requestType = REQUEST_PUT;
		}else if(strncmp(buffer, "DELETE", 6)	== 0){
			hInfo->requestType = REQUEST_DELETE;
		}else if(strncmp(buffer, "TRACE", 5)	== 0){
			hInfo->requestType = REQUEST_TRACE;
		}else if(strncmp(buffer, "OPTIONS", 7)	== 0){
			hInfo->requestType = REQUEST_OPTIONS;
		}else if(strncmp(buffer, "CONNECT", 7)	== 0){
			hInfo->requestType = REQUEST_CONNECT;
		}else if(strncmp(buffer, "PATCH", 5)	== 0){
			hInfo->requestType = REQUEST_PATCH;
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
		//TODO: error handling
		hInfo->isRange = 1;

		char *ptr = value + strlen(" bytes=");//FIXME, you guessing the number of spaces again !
		char *endPtr;
		hInfo->getContentRangeStart = strtol( ptr, &endPtr, 10);
		ptr = endPtr + 1;
		if (*ptr >= '0' && *ptr <= '9'){
			hInfo->getEndRangeSet = 1;
			hInfo->getContentRangeEnd = strtol( ptr, &endPtr, 10);
		}else{
			hInfo->getEndRangeSet = 0;
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
	//FIXME 0 out the offset
	//FIXME: This function can only ever return 0
	return 0; 
}
