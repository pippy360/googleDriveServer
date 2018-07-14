#ifndef UTILS_H
#define UTILS_H
#include "httpProcessing/commonHTTP.h"
#include "net/connection.h"
#include "google/googleAccessToken.h"
#include "httpProcessing/realtimePacketParser.h"
//char* strstrn(char* const haystack, const char* needle, const int haystackSize);

//void getConnectionByUrl(const char *inputUrl, Connection_t *httpConnection, char **domain);

int utils_parseUrl(const char *inputUrl, const char **protocol, int *protocolLength,
		const char **domain, int *domainLength, const char **fileUrl, int *fileUrlLength);

int utils_parseUrl_proto(const char *inputUrl, protocol_t *type, const char **domain,
		int *domainLength, const char **fileUrl, int *fileUrlLength);

int utils_recvNextHttpPacket(Connection_t *con, HTTPHeaderState_t *outputHInfo,
		char *outputBuffer, const int outputBufferMaxLength);

char *shitty_get_json_value(const char* inputName, char* jsonData, int jsonDataSize);

char *getAccessTokenHeader(AccessTokenState_t *tokenState);

int utils_downloadHTTPFileSimple(char *outputBuffer, const int outputMaxLength,
		char *inputUrl, HTTPHeaderState_t *hInfo, char *extraHeaders);

int utils_createHTTPHeaderFromUrl(char *inputUrl, char *output,
		int maxOutputLen, HTTPHeaderState_t *hInfo,
		const httpRequestTypes_t requestType, char *extraHeaders);

int utils_chunkData(const void *inputData, const int inputDataLength,
		void *outputBuffer);

void utils_connectByUrl(const char *inputUrl, Connection_t *con);

int utils_chunkAndSend(Connection_t *clientCon, char *dataBuffer,
		int dataBufferLen);

void utils_setHInfoFromUrl(const char *inputUrl, HTTPHeaderState_t *hInfo,
		const httpRequestTypes_t requestType, const char *extraHeaders);

#endif /* UTILS_H */