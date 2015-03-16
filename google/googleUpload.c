//todo create a header with all the max lenghts of the http stuff

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../commonDefines.h"
#include "../net/networking.h"
#include "../net/connection.h"
#include "googleUpload.h"
#include "googleAccessToken.h"
#include "../utils.h"

//0 if success, -1 otherwise
int googleUpload_init(Connection_t *con, AccessTokenState_t *accessTokenState,
		char *metadata, char *contentType) {

	char header[MAX_PACKET_SIZE];
	char extraHeaders[MAX_PACKET_SIZE];
	char metadataBuff[MAX_PACKET_SIZE];
	char postData[] = "Content-Type: application/json; charset=UTF-8\n\n"
			"{\n"
			"%s\n"
			"}\n"
			"--foo_bar_baz\n"
			"Content-Type: %s\n\n";
	headerInfo_t hInfo;

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

	//FIXME: USE A SEND ALL FUNCTION
	//send the header
	net_send(con, header, strlen(header));
	//and the meta data
	sprintf(metadataBuff, postData, metadata, contentType);
	net_send(con, metadataBuff, strlen(metadataBuff));

	return 0;
}

//0 if success, -1 otherwise
int googleUpload_update(Connection_t *con, char *dataBuffer, char* dataLength) {
	net_send(con, dataBuffer, dataLength);
	return 0;
}

//0 if success, -1 otherwise
int googleUpload_end(Connection_t *con) {
	char endData[] = "\n--foo_bar_baz--\n";
	net_send(con, endData, strlen(endData));
	//send the last chunk
	return 0;
}

