//todo: handle cases where send doesn't send all the bytes we asked it to
//todo: write packet logger !!!
//TODO: who closes the connection ?
//TODO: learn error handling
//"Note that the major and minor numbers MUST be treated as"
//"separate integers and that each MAY be incremented higher than a single digit."
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "commonDefines.h"
#include "net/networking.h"
#include "httpProcessing/commonHTTP.h"//TODO: parser states
#include "httpProcessing/realtimePacketParser.h"//TODO: parser states
#include "httpProcessing/createHTTPHeader.h"
#include "net/connection.h"
#include "google/googleAccessToken.h"
#include "utils.h"
#include "crypto.h"
#include "fileTransfer.h"
//#include "googleUpload.h"
//#include "parser.h"

#define MAX_ACCEPTED_HTTP_PAYLOAD 200000
#define MAXDATASIZE               200000//FIXME: this should only need to be the max size of one packet !!!
#define MAX_CHUNK_ARRAY_LENGTH    100
#define MAX_CHUNK_SIZE_BUFFER     100
#define SERVER_LISTEN_PORT        "25001"
#define FILE_SERVER_URL           "localhost"

void flipBits(void* packetData, int size) {
	char* b = packetData;
	int i;
	for (i = 0; i < size; ++i) {
		b[i] = ~(b[i]);
	}
}

void decryptPacketData(void* packetData, int size) {
	flipBits(packetData, size);
}

void getDownloadUrlAndSize(AccessTokenState_t *tokenState, char *filename,
		char **url, char **size) {

	char inputUrl[2000], *accessToken;
	char str[MAX_ACCEPTED_HTTP_PAYLOAD];
	HTTPHeaderState_t headerInfo;

	sprintf(inputUrl, "https://www.googleapis.com/drive/v2/files"
			"?q=title='%s'&fields=items(downloadUrl,fileSize)", filename);

	accessToken = utils_shittyGetAccessTokenHeader(tokenState);
	utils_downloadHTTPFileSimple(str, MAX_ACCEPTED_HTTP_PAYLOAD, inputUrl,
			&headerInfo, accessToken);

	//todo: check if we got the file

	//todo: check it the json has what we want

	printf("Json received --%s--", str);
	*url = shitty_get_json_value("downloadUrl", str, strlen(str));
	*size = shitty_get_json_value("fileSize", str, strlen(str));
	free(accessToken);
}

//pass in a Connectoin_t, and this will recv until you have the whole header
//it will discard any packet data
//returns 0 if success, non-0 otherwise
int getHeader(Connection_t *con, HTTPParserState_t *parserStateBuf,
		char *outputData, int outputDataMaxSize, int *outputDataLength,
		HTTPHeaderState_t *headerInfoBuf) {

	set_new_parser_state_struct(parserStateBuf);
	set_new_header_info(headerInfoBuf);

	char packetBuf[MAX_PACKET_SIZE + 1];
	while (!parserStateBuf->headerFullyParsed) {
		int recvd = net_recv(con, packetBuf, MAX_PACKET_SIZE);
		if(recvd == 0){
			return -1;
		}
		packetBuf[recvd] = '\0';

		printf("packet received from client: --%s--\n\n", packetBuf);

		process_data(packetBuf, recvd, parserStateBuf, outputData,
				outputDataMaxSize, outputDataLength, packetEnd_s,
				headerInfoBuf);
		//TODO: check for errors
	}
	return 0;
}

void createClientHeaderForDriveDownload(HTTPHeaderState_t *hInfoGoogle_response,
		HTTPHeaderState_t *hInfoClientRecv, int fileSize, char *packetBuffer) {

	hInfoGoogle_response->isRequest = 0;
	hInfoGoogle_response->transferType = TRANSFER_CHUNKED;
	hInfoGoogle_response->sentContentRangeStart =
			hInfoClientRecv->getContentRangeStart;
	hInfoGoogle_response->sentContentRangeEnd =
			(hInfoClientRecv->getEndRangeSet) ?
					hInfoClientRecv->getContentRangeEnd : fileSize;
	hInfoGoogle_response->sentContentRangeFull = fileSize;
	createHTTPHeader(packetBuffer, MAX_PACKET_SIZE, hInfoGoogle_response, NULL);
}

//FIXME: use a define for the blocksize variable
//the header has already been parsed by this point, hence we need to pass in output data
void downloadDriveFile(AccessTokenState_t *tokenState, Connection_t *clientCon,
		HTTPParserState_t *parserState, HTTPHeaderState_t *hInfoClientRecv,
		char *outputData, int outputDataLength) {

	int received, chunkSize, outputBufferLength;
	long fileSize;
	char *url, *sizeStr, *accessTokenHeaders;
	HTTPHeaderState_t hInfoGoogle_response, hInfoClient_response;
	HTTPParserState_t parserStateGoogle_response;
	Connection_t con;
	CryptoFileDownloadState_t encState;
	char packetBuffer[MAX_PACKET_SIZE], dataBuffer[MAX_PACKET_SIZE];

	/* get the url and size of the file using the name given by the url */
	getDownloadUrlAndSize(tokenState, hInfoClientRecv->urlBuffer + 1, &url,
			&sizeStr);
	fileSize = strtol(sizeStr, NULL, 10);
	printf("File found, size: %s, url: %s\n", sizeStr, url);

	accessTokenHeaders = utils_shittyGetAccessTokenHeader(tokenState);
	printf("geting ready to start file download\n");
	startEncryptedFileDownload(&encState, url, hInfoClientRecv->isRange,
			hInfoClientRecv->getEndRangeSet,
			hInfoClientRecv->getContentRangeStart,
			hInfoClientRecv->getContentRangeEnd, &con, &hInfoGoogle_response,
			&parserStateGoogle_response, accessTokenHeaders);
	printf("about to call update\n");
	/* get the header */
	while (!parserStateGoogle_response.headerFullyParsed) {
		updateEncryptedFileDownload(&encState, &con, &hInfoGoogle_response,
				&parserStateGoogle_response, dataBuffer, MAX_PACKET_SIZE,
				&outputBufferLength, accessTokenHeaders);
	}
	printf("done with update\n");
	/* reply to the client about the file */
	createClientHeaderForDriveDownload(&hInfoGoogle_response, hInfoClientRecv,
			fileSize, packetBuffer);
	net_send(clientCon, packetBuffer, strlen(packetBuffer));

	/*chunk and send any data we received while receiving the header*/
	utils_chunkAndSend(clientCon, dataBuffer, outputBufferLength);

	/*break if client or google closes the connection ?*/
	while (updateEncryptedFileDownload(&encState, &con, &hInfoGoogle_response,
				&parserStateGoogle_response, dataBuffer, MAX_PACKET_SIZE,
				&outputBufferLength, accessTokenHeaders) != 0){
		//printf("update finished, we got this much data: %d\n", outputBufferLength);
		if(utils_chunkAndSend(clientCon, dataBuffer, outputBufferLength) == -1){
			printf("client closed the connection\n");
			break;
		}
	}
	finishEncryptedFileDownload(&encState);

	//chunk the finished stuff

	/*clean up everything not covered by finish file download*/
	net_send(clientCon, "0\r\n\r\n", strlen("0\r\n\r\n"));
	printf("we've finished everything\n");
	net_close(&con);
}

void handle_client(AccessTokenState_t *stateStruct, int client_fd) {

	/* get the first packet from the client,
	 continue until we have the whole header and see what he wants */

	int outputDataLength;
	char *outputDataBuffer = malloc(MAXDATASIZE + 1);
	HTTPParserState_t parserStateClientRecv;
	HTTPHeaderState_t hInfoClientRecv;
	Connection_t httpCon;
	net_fileDescriptorToConnection(client_fd, &httpCon);

	if(getHeader(&httpCon, &parserStateClientRecv, outputDataBuffer,
			MAXDATASIZE, &outputDataLength, &hInfoClientRecv) != 0){
		printf("client closed connection while we were getting the header\n");
		return;
	}

	/* check what the client wants by checking the URL*/
	if (!strncmp(hInfoClientRecv.urlBuffer, "/pull/", strlen("/pull/"))) {
		//FIXME: quick hack here
		printf("we're downloading a file\n");
		hInfoClientRecv.urlBuffer = hInfoClientRecv.urlBuffer + strlen("/pull");
		downloadDriveFile(stateStruct, &httpCon, &parserStateClientRecv,
				&hInfoClientRecv, outputDataBuffer, outputDataLength);
	} else if (!strncmp(hInfoClientRecv.urlBuffer, "/push/",
			strlen("/push/"))) {
		//printf("NOT NOT NOT starting file download !\n");

	} else {
		//404 !
		printf("favicon?\n");
	}

	printf("we're done apparently\n");
	close(client_fd);
}

int main(int argc, char *argv[]) {
//google_init();
	AccessTokenState_t stateStruct;
	gat_init_googleAccessToken(&stateStruct);

	int sockfd = getListeningSocket(SERVER_LISTEN_PORT);
	int client_fd;
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];

	printf("server: waiting for connections...\n");

	while (1) {  // main accept() loop
		sin_size = sizeof their_addr;
		client_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if (client_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			handle_client(&stateStruct, client_fd);

			close(client_fd);
			exit(0);
		}
		close(client_fd);  // parent doesn't need this
	}

	return 0;
}
