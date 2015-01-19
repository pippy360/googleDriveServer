#define HTTP_VERSION "HTTP/1.1"

typedef enum {
    PROTO_HTTP,
    PROTO_HTTPS
} protocol_t;

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
	default_empty,//a packet with no body/payload
	TRANSFER_CHUNKED,
	TRANSFER_CONTENT_LENGTH
} packetDataTypes_t;

typedef struct {
	long getContentRangeStart;
	long getContentRangeEnd;
	char getContentRangeEndSet;//a bool to check if we got a "getContentRangeEnd"
	long sentContentRangeStart;
	long sentContentRangeEnd;
	long sentContentRangeFull;
	long contentLength;
	packetDataTypes_t transferType;
	httpRequestTypes_t requestType;
	char isRequest;
	char isRange;
	int  statusCode;
	char *statusStringBuffer;
	char *urlBuffer;//
	char *hostBuffer;//used for the host header
} headerInfo_t;

