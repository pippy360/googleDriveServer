







typedef enum {
	getHeaderFirstLine,//next state -> finishHeaderValue
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
	packetEnd,
	BAD_PACKET
} state_t;

typedef enum {
	chunked,
	contentLength
} packetDataTypes_t;

typedef struct {
	packetDataTypes_t type;// 1 - content-length, 2 - chunked
	unsigned long contentLength;
} headerInfo;

//state of the parser
typedef struct {
	state_t currentState;
	state_t nextState;
	char *currentPacketPtr;
	headerInfo *header;
	int stateEndIdentifier;
	int fullLength;
	unsigned long remainingLength;
	int currentTokenPos;
	packetDataTypes_t transferEncoding; 
	/*lengthBuffer used to buffer lengths, because the declared 
	length could be stretched across multiple packets*/
	char *lengthBuffer;
	char *nameBuffer;
	char *valueBuffer;
} parserStateStuct;

int process_data(char *inputData, int dataLength, int offset, parserStateStuct* state, 
				char *outputData, int outputDataMaxLength, int *outputDataLength, state_t exitState);

parserStateStuct* get_start_state_struct();
