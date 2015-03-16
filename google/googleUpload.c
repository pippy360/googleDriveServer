//todo create a header with all the max lenghts of the http stuff

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../commonDefines.h"
#include "../net/networking.h"
#include "../net/connection.h"
#include "../httpProcessing/commonHTTP.h"//TODO: parser states
#include "../httpProcessing/realtimePacketParser.h"//TODO: parser states
#include "../httpProcessing/createHTTPHeader.h"

#include "googleAccessToken.h"
#include "googleUpload.h"
#include "../utils.h"


//0 if success, -1 otherwise
int googleUpload_init(Connection_t *con, AccessTokenState_t *accessTokenState,
		char *metadata, char *contentType) {
	printf("segfault test 0\n");
	char header[MAX_PACKET_SIZE];
	char extraHeaders[MAX_PACKET_SIZE];
	char metadataBuff[MAX_PACKET_SIZE];
	char postData[] = "--foo_bar_baz\nContent-Type: application/json; charset=UTF-8\n\n"
			"{\n"
			"%s\n"
			"}\n"
			"--foo_bar_baz\n"
			"Content-Type: %s\n\n";
	headerInfo_t hInfo;
	set_new_header_info(&hInfo);
	hInfo.transferType = TRANSFER_CHUNKED;
	//Authorization: Bearer your_auth_token
	//Content-Type: multipart/related; boundary="foo_bar_baz"
	sprintf(extraHeaders, "Authorization: Bearer %s\r\n"
			"Content-Type: multipart/related; boundary=\"foo_bar_baz\"\r\n",
			accessTokenState->accessTokenStr);

	utils_connectByUrl("https://www.googleapis.com"
			"/upload/drive/v2/files?uploadType=multipart", con);

	utils_createHTTPHeaderFromUrl("https://www.googleapis.com"
			"/upload/drive/v2/files?uploadType=multipart", header,
			MAX_PACKET_SIZE, &hInfo, REQUEST_POST, extraHeaders);

	char outputData[MAX_PACKET_SIZE];
	int outputDataLength;

	//FIXME: USE A SEND ALL FUNCTION
	//send the header
	net_send(con, header, strlen(header));


	//and the meta data
	sprintf(metadataBuff, postData, metadata, contentType);
	outputDataLength = utils_chunkData(metadataBuff, strlen(metadataBuff), outputData);
	net_send(con, outputData, outputDataLength);

	return 0;
}

//0 if success, -1 otherwise
int googleUpload_update(Connection_t *con, char *dataBuffer, int dataLength) {
	char outputData[MAX_PACKET_SIZE];
	int outputDataLength = utils_chunkData(dataBuffer, dataLength, outputData);
	net_send(con, outputData, outputDataLength);
	return 0;
}

//0 if success, -1 otherwise
int googleUpload_end(Connection_t *con) {
	char endData[] = "\n--foo_bar_baz--";
	char outputData[MAX_PACKET_SIZE];
	int outputDataLength = utils_chunkData(endData, strlen(endData), outputData);
	net_send(con, outputData, outputDataLength);
	net_send(con, "0\r\n\r\n", strlen("0\r\n\r\n"));
	//send the last chunk
	int r = net_recv(con, outputData, MAX_PACKET_SIZE);
	printf("reply from google %.*s\n", r, outputData);
	return 0;
}

