#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "httpProcessing/commonHTTP.h"
#include "httpProcessing/realtimePacketParser.h"
#include "net/networking.h"
#include "net/connection.h"
#include "commonDefines.h"

char* strstrn(char* const haystack, const char* needle, const int haystackSize) {
	if (haystackSize < strlen(needle))
		return NULL;

	int i;
	for (i = 0; i < (haystackSize - (strlen(needle) - 1)); i++) {
		int j;
		for (j = 0; j < strlen(needle); j++)
			if (needle[j] != haystack[i + j])
				break;
		if (j == strlen(needle))
			return haystack + i;
	}
	return NULL;
}

//FIXME: this shouldn't be case sensitive
//just like utils_parseUrl but it gives you the protocol in type protocol_t
//returns 0 if valid url, -1 otherwise
//TODO: should work with lower case too and there shouldn't be so much hardcoded stuff
int utils_parseUrl_proto(char *inputUrl, protocol_t *type, char **domain,
		int *domainLength, char **fileUrl, int *fileUrlLength) {

	int protoSize, protocolLength, returnCode;
	char *protocolStr;
	returnCode = utils_parseUrl(inputUrl, &protocolStr, &protocolLength, domain,
			domainLength, fileUrl, fileUrlLength);
	if (returnCode == 0) {
		return 0;
	}
	protoSize = protocolLength;

	if (strncmp(protocolStr, "https",
			(protoSize > strlen("https")) ? protoSize : strlen("https")) == 0) {
		*type = PROTO_HTTPS;
	} else if (strncmp(protocolStr, "http",
			(protoSize > strlen("http")) ? protoSize : strlen("http")) == 0) {
		*type = PROTO_HTTP;
	} else {
		printf("ERROR: BAD PROTOCOL\n");
		*type = PROTO_HTTP;
	}
	return 1;
}

//returns 1 if valid url, 0 otherwise
//fileUrl can be 0 in length
//fileUrl is everything after ".com"
//inputUrl must be a null terminated string
int utils_parseUrl(char *inputUrl, char **protocol, int *protocolLength,
		char **domain, int *domainLength, char **fileUrl, int *fileUrlLength) {
	int remaining;
	int urlLength = strlen(inputUrl);
	char *dotPtr, *protoEndPtr, *fuPtr;

	//get protocol
	//go till '://'
	protoEndPtr = strstrn(inputUrl, "://", strlen(inputUrl));
	if (protoEndPtr == NULL) {
		return 0;
	}
	*protocol = inputUrl;
	*protocolLength = protoEndPtr - inputUrl;

	*domain = protoEndPtr + strlen("://");
	remaining = strlen(inputUrl) - (*domain - inputUrl);

	//get the first occurrence of a "."
	dotPtr = strstrn(*domain, ".", remaining);
	if (dotPtr == NULL) {
		return 0;
	}

	//now try to get the first occurrence of a single "/"
	*fileUrl = strstrn(dotPtr, "/", remaining);

	//if that failed then the url doesn't end in a "/"
	//and the fileUrlLength will be zero
	if (*fileUrl == NULL) {
		*fileUrl = inputUrl + strlen(inputUrl);
		*fileUrlLength = 0;
		*domainLength = strlen(inputUrl) - (*domain - inputUrl);
	} else {
		*fileUrlLength = strlen(inputUrl) - (*fileUrl - inputUrl);
		*domainLength = (*fileUrl - *domain);
	}
	return 1;
}

void utils_getAccessTokenHeader() {
//    getAccessToken();
//    char headerStub[]  = "Authorization: Bearer ";
//
//    char* tokenHeader = malloc( strlen(headerStub) + strlen(accessToken) + 1 + 2 );
//    sprintf( tokenHeader, "%s%s%s", headerStub, accessToken, "\r\n");
//
//    return tokenHeader;
}


//creates a hIfno get request for the the url
void utils_setHInfoFromUrl(char *inputUrl, headerInfo_t *hInfo,
		const httpRequestTypes_t requestType, char *extraHeaders) {
	protocol_t type;
	char *fileUrl, *domain;
	int fileUrlLength, domainLength;

	utils_parseUrl_proto(inputUrl, &type, &domain, &domainLength, &fileUrl,
			&fileUrlLength);
	hInfo->isRequest = 1;
	hInfo->requestType = requestType;
	//FIXME: there should really be some sort of setUrlBuffer and setHostBuffer
	memcpy(hInfo->urlBuffer, fileUrl, fileUrlLength);
	memcpy(hInfo->hostBuffer, domain, domainLength);
}


//FIXME: CONST THIS STUFF !
void getConnectionStructByUrl(char *inputUrl, Connection_t *con) {
	protocol_t type;
	char *fileUrl, *domain;
	int fileUrlLength, domainLength;

	utils_parseUrl_proto(inputUrl, &type, &domain, &domainLength, &fileUrl,
			&fileUrlLength);
	if (type == PROTO_HTTP) {
		con->type = PROTO_HTTP;
	} else if (type == PROTO_HTTPS) {
		con->type = PROTO_HTTPS;
	}
}

//returns an active tcp connection to the url 
void utils_connectByUrl(char *inputUrl, Connection_t *con) {
	protocol_t type;
	char *fileUrl, *domain;
	char domainBuffer[MAX_DOMAIN_SIZE];
	int fileUrlLength, domainLength;

	net_setupNewConnection(con);
	getConnectionStructByUrl(inputUrl, con);

	utils_parseUrl_proto(inputUrl, &type, &domain, &domainLength, &fileUrl,
			&fileUrlLength);
	sprintf(domainBuffer, "%.*s", domainLength, domain);
	net_connect(con, domainBuffer);
}

//This function allows the content length to already be set,
//if the content length is not set then the packet must not have any data (other than the header)
int utils_createHTTPHeaderFromUrl(char *inputUrl, char *output,
		int maxOutputLen, headerInfo_t *hInfo,
		const httpRequestTypes_t requestType, char *extraHeaders) {

	utils_setHInfoFromUrl(inputUrl, hInfo, requestType, extraHeaders);
	printf("hInfo set %lu, %d\n", hInfo->contentLength, hInfo->transferType);
	return createHTTPHeader(output, maxOutputLen, hInfo, extraHeaders);
}

//FIXME: handle cases where the outputBuffer isn't big enough
//returns the amount of data written to outputData
int utils_recvNextHttpPacket(Connection_t *con, headerInfo_t *outputHInfo,
		char *outputBuffer, const int outputBufferMaxLength) {

	char *tempPtr = outputBuffer;
	char packetBuffer[MAX_PACKET_SIZE];
	int received, outputDataLength;
	parserState_t parserState;

	set_new_header_info(outputHInfo);
	set_new_parser_state_struct(&parserState);

	while (parserState.currentState != packetEnd_s) {

		received = net_recv(con, packetBuffer, MAX_PACKET_SIZE);
		printf("the output data so far: --%.*s--\n", received, packetBuffer);
		process_data(packetBuffer, received, &parserState, tempPtr,
				10000, &outputDataLength, packetEnd_s, outputHInfo);
		tempPtr += outputDataLength;
	}
	return tempPtr - outputBuffer;
}

//returns the amount of data downloaded (excluding the header)
int utils_downloadHTTPFileSimple(char *outputBuffer, const int outputMaxLength,
		char *inputUrl, headerInfo_t *hInfo, char *extraHeaders) {

	parserState_t parserState;
	Connection_t con;
	char packetBuffer[MAX_PACKET_SIZE];

	set_new_parser_state_struct(&parserState);
	set_new_header_info(hInfo);

	utils_connectByUrl(inputUrl, &con);

	//request the file
	headerInfo_t requestHeaderInfo;
	utils_createHTTPHeaderFromUrl(inputUrl, packetBuffer, MAX_PACKET_SIZE,
			&requestHeaderInfo, REQUEST_GET, extraHeaders);
	net_send(&con, packetBuffer, strlen(packetBuffer));

	return utils_recvNextHttpPacket(&con, hInfo, outputBuffer, outputMaxLength);
}
