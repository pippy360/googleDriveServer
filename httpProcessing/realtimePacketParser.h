







typedef enum {
	getHTTPStatusLine,//next state -> finishHeaderValue
	finishedHTTPStatusLine,
	getHeaderName,
	finishedHeaderName,//expecting ':' 
	getHeaderValue,
	finishedHeaderValue,//TODO:EXPLAIN
	finishedHeaderValue_2,//TODO:EXPLAIN
	getChunkLength,
	finishedChunkLength,//expecting '\r\n'
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
} parserState_t;

int process_data(char *inputData, int dataLength, parserState_t* state, char *outputData, 
				int outputDataMaxLength, int *outputDataLength, state_t exitState, headerInfo_t *hInfo);

void set_new_parser_state_struct(parserState_t *parserState);

void set_new_header_info(headerInfo_t *hInfo);