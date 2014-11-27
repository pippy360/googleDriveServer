//parses the header and chunked packets and content-length packets

//RENAME TO HTTP PACKET PARSER

//NOTE: this doesn't EVER buffer anything !!!

//TODO: STATE ENUM

//TODO: create an option to quit once we get to a state
 
typedef enum {
	getHeaderFirstLine,//next state -> finishHeaderValue
	getHeaderName,
	finishedHeaderName,//expecting ':' 
	getHeaderValue,
	finishedHeaderValue,//expeccting '\r\n' or '\r\n\r\n'
	getChunkLength,
	finishedChunkLength,//expecting '\r\n'
	getChunkData,
	finishedChunkData,//expecting '\r\n' followed by new chunk or zero chunk
	getContentLengthData,
	packetEnd,
	BAD_PACKET
} state_t;

//state of the parser
typedef struct {
	state_t currentState;
	state_t nextState;
	char *currentPacketPtr;
	headerInfo *header;
	int stateEndIdentifier;
	int fullLength;
	int remainingLength;
	char *endToken;
	char *currentTokenPos;
	/*lengthBuffer used to buffer lengths, because the declared 
	length could be stretched across multiple packets*/
	char *lengthBuffer;
	char *nameBuffer;
} parserStateStuct;

typedef enum {
	chunked,
	contentLength
} packetDataTypes_t;

typedef struct {
	packetDataTypes_t type;// 1 - content-length, 2 - chunked
	unsigned long contentLength;
} headerInfo;


char* get_start_state_struct(){

}

void setState(parserStateStuct *state, state_t nextState){
	//on state change
	//set the tokens and stuff

	switch ( nextState ){
		case getHeaderFirstLine:
			state->currentState = nextState;
			break;
		case getHeaderName:
			state->currentState = nextState;
			break;
		case getHeaderValue:
			state->currentState = nextState;
			break;
		case finishedHeaderValue:
			state->currentState = nextState;
			break;
		case finishedHeaderValue_2:
			state->currentState = nextState;
			break;
		case getChunkLength: getChunkLength_firstNumber ){
			state->currentState = nextState;
			break;
		case getChunkData:
			state->currentState = nextState;
			break;
		case getContentLengthData:
			state->currentState = nextState;
			break;
	}
}

//0 == finished getting a valid token, 1 == still going through it, -1 == nope, failed
void token_check(parserStateStuct *state, char *outputBuffer, char *token){
	if( state->currentPacketPtr[0] == state->endToken[ state->currentTokenPos ] ){
		if ( currentTokenPos == strlen( state->endToken ) - 1 )
			return 0;
		else
			return 1;
	}else{
		return -1;
	}
}

//this can only get hardcoded headers, to get more headers use a getHeader function on the fully downloaded header
void process_byte(char byte, packetStateStuct* state, 
				char *outputData, int *outputDataMaxLength, int *outputDataLength){
	//grab any info about the things
	//add to header name buffer, once a name has finished

	//TODO: use a switch statement

	int retVal;
	char currByte = state->currentPacketPtr[0];
	switch ( state->currentState ){

		case getHeaderFirstLine:
			if( (retVal = token_check(state, NULL, "\r\n")) == 0 ){
				setState( finishedHeaderValue, state );
			}else{
				
			}
			break;
		case getHeaderName:
			if( (retVal = token_check(state, NULL, ":")) == 0 ){
				setState( getHeaderValue, state );
			}else{
				//add to the header buffer
			}
			break;
		case getHeaderValue:
			if( (retVal = token_check(state, NULL, "\r\n")) == 0 ){				
				setState( finishedHeaderValue, state );
				//pop the chars off the buffer !!
			}
			//add the char to the buffer
			break;
		case finishedHeaderValue:
			if( currByte == '\r' ){
				setState( finishedHeaderValue_2, state );
			}else if( 1/*it's a vaild character*/ ){	
				//add to the buffer
				//set state
				setState( getHeaderName, state );
			}else{
				//fail
			}
			break;
		case finishedHeaderValue_2:
			if( currByte == '\n' ){
				//check if it's chunked 
				setState( getChunkLength, state );
			}else{
				//fail
			}
			break;
		case getChunkLength: getChunkLength_firstNumber ){
			if( (retVal = token_check("\r\n")) == 0 ){
				
			}
			break;
		case getChunkData:
			//length check and if we've reached the length then token check until we reach the end
			break;
		case getContentLengthData:
			if(/* the length */){
				///add to buffer
			}
			break;
	}

	state->currentPacketPtr++;
}

//call this when ever we get a new packet
//TODO: ADD A FUNCTION TO EXIT ON STATE
int process_data(char *inputData, int dataLength, int offset, packetStateStuct* state, 
				char *outputData, int *outputDataMaxLength, int *outputDataLength){
	while( state->currentPacketPtr != inputData+dataLength ){
		processByte(inputData, dataLength, offset, state, 
					outputData, outputDataMaxLength, outputDataLength);
		//everytime we hit
		if( state->currentState == getChunkLength )
			break;
	}
}
