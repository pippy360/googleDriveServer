#ifndef COMMONHTTP_H
#define COMMONHTTP_H

#define HTTP_VERSION "HTTP/1.1"

typedef enum {
    PROTO_HTTP,
    PROTO_HTTPS
} protocol_t;

typedef enum {
	REQUEST_GET,
	REQUEST_HEAD,
	REQUEST_POST,
	REQUEST_PUT,
	REQUEST_DELETE,
	REQUEST_TRACE,
	REQUEST_OPTIONS,
	REQUEST_CONNECT,
	REQUEST_PATCH
} httpRequestTypes_t;

typedef enum {
	default_empty,//a packet with no body/payload
	TRANSFER_CHUNKED,
	TRANSFER_CONTENT_LENGTH
} packetDataTypes_t;

//TODO: FIXME: this contentRange stuff is a mess, fix it !
typedef struct {
	long getContentRangeStart;
	long getContentRangeEnd;
	char getEndRangeSet;//if this value is 0 then the format is Range: bytes=X-\r\n
	long sentContentRangeStart;
	long sentContentRangeEnd;
	long sentContentRangeFull;
	long contentLength;
	packetDataTypes_t transferType;
	httpRequestTypes_t requestType;
	char isRequest;
	char isRange;//FIXME: is this true for both ranged requests and responses ?
	int  statusCode;
	char *statusStringBuffer;
	char *urlBuffer;//
	char *hostBuffer;//used for the host header
} HTTPHeaderState_t;

#endif /* COMMONHTTP_H */