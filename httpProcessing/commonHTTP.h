#define HTTP_VERSION "HTTP/1.1"

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
	chunked,
	contentLength
} packetDataTypes_t;

typedef struct {
	long getContentRangeStart;
	long getContentRangeEnd;
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
	char *urlBuffer;
} headerInfo_t;

