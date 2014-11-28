//parses the header and chunked packets and content-length packets
//RENAME TO HTTP PACKET PARSER
//NOTE: this doesn't EVER buffer anything !!!
//TODO: STATE ENUM
//TODO: create an option to quit once we get to a state
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "realtimePacketParser.h"

#define MAX_LEN 10000
#define MAX_BUFFER 1000//FIXME: do something to prevent bufferoverflows

//FIXME: THE BUFFERS SHOULDN'T BE MALLOC'D HERE
parserStateStuct* get_start_state_struct(){
	parserStateStuct *state = calloc( sizeof( parserStateStuct ), 1 );
	state->currentState = 0;
	state->currentPacketPtr = NULL;
	state->lengthBuffer 	= malloc(MAX_BUFFER+1);
	state->lengthBuffer[0] 	= '\0';
	state->nameBuffer 		= malloc(MAX_BUFFER+1);
	state->nameBuffer[0] 	= '\0';
	state->valueBuffer 		= malloc(MAX_BUFFER+1);
	state->valueBuffer[0] 	= '\0';

	return state;
}

void setState(parserStateStuct *state, state_t nextState){
	state->currentTokenPos = 0;
	state->currentState = nextState;
}

//0 == finished getting a valid token, 1 == still going through it, -1 == nope, failed
int token_check(parserStateStuct *state, char *outputBuffer, char *token){
	int result;
	if( state->currentPacketPtr[0] == token[ state->currentTokenPos ] ){
		if ( state->currentTokenPos == strlen( token ) - 1 )
			result = 0;
		else
			result = 1;
		state->currentTokenPos++;
	}else{
		result = -1;
		state->currentTokenPos = 0;
	}
	return result;
}

//this can only get hardcoded headers, to get more headers use a getHeader function on the fully downloaded header
void process_byte(char byte, parserStateStuct* state, 
				char *outputData, int outputDataMaxLength, int *outputDataLength){

	int retVal, length;
	char currByte = state->currentPacketPtr[0];
	switch ( state->currentState ){

		case getHeaderFirstLine:
			if( (retVal = token_check(state, NULL, "\r\n")) == 0 ){
				//YOU NEED TO POP THE \R
				setState( state, finishedHeaderValue );
			}else{
			}
			break;
		case getHeaderName:
			if( (retVal = token_check(state, NULL, ":")) == 0 ){
				setState( state, getHeaderValue );
			}else{
				//FIXME: CHECK FOR BUFFER OVERFLOW
				length = strlen( state->nameBuffer );
				state->nameBuffer[length] = currByte;
				state->nameBuffer[length+1] = '\0';
			}
			break;
		case getHeaderValue:
			if( (retVal = token_check(state, NULL, "\r\n")) == 0 ){				
				length = strlen( state->valueBuffer );
				state->valueBuffer[length-1] = '\0';
				setState( state, finishedHeaderValue );
			}else{
				length = strlen( state->valueBuffer );
				state->valueBuffer[length] = currByte;
				state->valueBuffer[length+1] = '\0';
			}
			break;
		case finishedHeaderValue:
			if( currByte == '\r' ){
				setState( state, finishedHeaderValue_2 );
			}else if( 1/*it's a vaild character*/ ){	
				//FIXME: this is kind of dodgy clearing the buffers here
				state->nameBuffer[0] = currByte;
				state->nameBuffer[1] = '\0';
				state->valueBuffer[0] = '\0';
				setState( state, getHeaderName );
			}else{
				//fail
			}
			break;
		case finishedHeaderValue_2:
			if( currByte == '\n' ){
				//check if it's chunked 
				if (state->transferEncoding == chunked)
				{
					setState( state, getChunkLength );
				}else if (state->transferEncoding == contentLength){
					setState( state, getContentLengthData );
				}else{
					//fail
				}
			}else{
				//fail
			}
			break;
		case getChunkLength:
			if( (retVal = token_check(state, NULL, "\r\n")) == 0 ){
				state->remainingLength = strtol(state->lengthBuffer, NULL, 16);
				if( state->remainingLength == 0 ){	
					setState( state, packetEnd );
				}else{
					setState( state, getChunkData );
				}
			}else if(retVal == 1){
			}else{
				length = strlen( state->lengthBuffer );
				state->lengthBuffer[length] = currByte;
				state->lengthBuffer[length+1] = '\0';
			}
			break;
		case getChunkData:
			outputData[*outputDataLength] = currByte;
			outputData[*outputDataLength+1] = '\0';
			(*outputDataLength)++;
			if ( state->remainingLength == 1 )
			{
				setState( state, finishedChunkData );
				state->lengthBuffer[0] = '\0';
			}
			state->remainingLength--;
			break;
		case finishedChunkData:
			if( (retVal = token_check(state, NULL, "\r\n")) == 0 ){				
				setState( state, getChunkLength );
			}else if(retVal != 1){
				//fail
			}
			break;
		case getContentLengthData:
			outputData[*outputDataLength] = currByte;
			outputData[*outputDataLength+1] = '\0';
			(*outputDataLength)++;
			if( state->remainingLength == 1 ){
				setState( state, packetEnd );
			}
			state->remainingLength--;
			break;
		case packetEnd:
			printf("hm.....hit packet end\n");
			break;
	}
}

//call this when ever we get a new packet
//TODO: ADD A FUNCTION TO EXIT ON STATE
int process_data(char *inputData, int dataLength, int offset, parserStateStuct* state, 
				char *outputData, int outputDataMaxLength, int *outputDataLength, state_t exitState){

	state_t prevState;
	*outputDataLength = 0;
	while( state->currentPacketPtr != inputData+dataLength ){
		prevState = state->currentState;
		
		process_byte( state->currentPacketPtr[0], state, outputData,
					 outputDataMaxLength, outputDataLength);

		//printf("state: %d\n", state->currentState);

		if (state->currentState != prevState && state->currentState == finishedHeaderValue){
			if(strcmp("Transfer-Encoding",state->nameBuffer) == 0 
				&& strcmp(" chunked",state->valueBuffer) == 0){
				printf("it's chunked\n");
				state->transferEncoding = chunked;
			}else if(strcmp("Content-Length",state->nameBuffer) == 0){
				state->transferEncoding = contentLength;
				state->remainingLength = strtol(state->valueBuffer, NULL, 10);
				printf("content length : %lu\n", state->remainingLength);
			}
		}
		//TODO: have an error if it's not content-length, chunked

		if (state->currentState != prevState){
			printf("state changed: %d -> %d\n", prevState, state->currentState );
		}

		if( state->currentState == exitState )
			break;

		state->currentPacketPtr++;
	}
}

