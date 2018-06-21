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
	char header[MAX_PACKET_SIZE];
	char extraHeaders[MAX_PACKET_SIZE];
	char metadataBuff[MAX_PACKET_SIZE];
	char postData[] =
		"--foo_bar_baz\nContent-Type: application/json; charset=UTF-8\n\n"
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

	utils_createHTTPHeaderFromUrl(
			"https://www.googleapis.com"
					"/upload/drive/v2/files?uploadType=multipart"
					"&fields=title,fileSize,fileExtension,id,downloadUrl,webContentLink",
			header, MAX_PACKET_SIZE, &hInfo, REQUEST_POST, extraHeaders);

	char outputData[MAX_PACKET_SIZE];
	int outputDataLength;

	//FIXME: USE A SEND ALL FUNCTION
	//send the header
	net_send(con, header, strlen(header));

	//and the meta data
	sprintf(metadataBuff, postData, metadata, contentType);
	outputDataLength = utils_chunkData(metadataBuff, strlen(metadataBuff),
			outputData);
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
int googleUpload_end(Connection_t *con, GoogleUploadState_t *fileState) {
	char endData[] = "\n--foo_bar_baz--";
	char outputData[MAX_PACKET_SIZE];
	headerInfo_t hInfo;
	int received;
	int outputDataLength = utils_chunkData(endData, strlen(endData),
			outputData);
	net_send(con, outputData, outputDataLength);
	//send the last chunk
	net_send(con, "0\r\n\r\n", strlen("0\r\n\r\n"));

	//now get all the return data and store it in
	//FIXME: this output buffer really isn't big enough
	received = utils_recvNextHttpPacket(con, &hInfo, outputData,
			MAX_PACKET_SIZE);
	printf("received:\n%s\n", outputData);	
	//you have to save this data !!
	printf("the id of the uploaded file is: %s\n",
			shitty_get_json_value("id", outputData, received));
	fileState->id = shitty_get_json_value("id", outputData, received);
	fileState->webUrl = shitty_get_json_value("webContentLink", outputData,
			received);
	fileState->apiUrl = shitty_get_json_value("downloadUrl", outputData,
			received);

	return 0;
}

