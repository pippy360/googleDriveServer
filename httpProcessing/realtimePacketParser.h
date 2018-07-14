#ifndef REALTIMEPACKETPARSER
#define REALTIMEPACKETPARSER

typedef enum {
	getHTTPStatusLine,//next state -> finishHeaderValue
	finishedHTTPStatusLine,
	getHeaderName,
	getHeaderValue,
	finishedHeaderValue,//TODO:EXPLAIN
	finishedHeaderValue_2,//TODO:EXPLAIN
	getChunkLength,
	getChunkData,
	finishedChunkData,//expecting '\r\n' followed by new chunk or zero chunk
	getContentLengthData,
	packetEnd_s,
	BAD_PACKET
} state_t;

typedef enum {
	default_empty_p,//a packet with no body/payload
	chunked_p,
	contentLength_p
} parserPacketDataType_t;

//state of the parser
typedef struct {
	state_t currentState;
	char *currentPacketPtr;
	int stateEndIdentifier;
	int fullLength;
	unsigned long remainingLength;
	int currentTokenPos;
	/* lengthBuffer used to buffer lengths, because the "Content-length"
	could be stretched across multiple packets */
	char *lengthBuffer;
	char *nameBuffer;
	char *valueBuffer;
	parserPacketDataType_t packetDataType;
	char *statusLineBuffer;
	int  headerFullyParsed;
} HTTPParserState_t;

int process_data(char *inputData, int dataLength, HTTPParserState_t* state, char *outputData, 
				int outputDataMaxLength, int *outputDataLength, state_t exitState, HTTPHeaderState_t *hInfo);

void set_new_parser_state_struct(HTTPParserState_t *parserState);

void set_new_header_info(HTTPHeaderState_t *hInfo);


#endif /* REALTIMEPACKETPARSER */
