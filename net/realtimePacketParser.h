







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
	packetEnd,
	BAD_PACKET
} state_t;

typedef enum {
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	TRACE,
	OPTIONS,
	CONNECT,
	PATCH
} httpRequestTypes_t;

typedef enum {
//todo:
	todo
} httpErrorCodes_t;

typedef enum {
	chunked,
	contentLength,
	contentRange,
	default_empty//a packet with no body/payload
} packetDataTypes_t;

typedef struct {
	long contentRangeStart;
	long contentRangeEnd;
	long contentLength;
	packetDataTypes_t transferType;
	httpRequestTypes_t requestType;
	char isRequest;
	int  statusCode;
	char *statusStringBuffer;
	char *urlBuffer;
} headerInfo_t;

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
	packetDataTypes_t transferType;
	char *statusLineBuffer;
} parserState_t;

int process_data(char *inputData, int dataLength, parserState_t* state, char *outputData, 
				int outputDataMaxLength, int *outputDataLength, state_t exitState, headerInfo_t *hInfo);

parserState_t* get_start_state_struct();

headerInfo_t *get_start_header_info();