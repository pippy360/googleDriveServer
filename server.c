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
	headerInfo_t headerInfo;

	sprintf(inputUrl, "https://www.googleapis.com/drive/v2/files"
			"?q=title='%s'&fields=items(downloadUrl,fileSize)", filename);

	accessToken = getAccessTokenHeader(tokenState);
	utils_downloadHTTPFileSimple(str, MAX_ACCEPTED_HTTP_PAYLOAD, inputUrl,
			&headerInfo, accessToken);

	//todo: check if we got the file

	//todo: check it the json has what we want

	printf("Json received --%s--", str);
	*url = shitty_get_json_value("downloadUrl", str, strlen(str));
	*size = shitty_get_json_value("fileSize", str, strlen(str));
	free(accessToken);
}

//returns 0 if success, non-0 otherwise
int getHeader(Connection_t *con, parserState_t *parserStateBuf,
		char *outputData, int outputDataMaxSize, int *outputDataLength,
		headerInfo_t *headerInfoBuf) {

	set_new_parser_state_struct(parserStateBuf);
	set_new_header_info(headerInfoBuf);

	char packetBuf[MAX_PACKET_SIZE + 1];
	while (!parserStateBuf->headerFullyParsed) {
		int recvd = net_recv(con, packetBuf, MAX_PACKET_SIZE);
		packetBuf[recvd] = '\0';
		process_data(packetBuf, recvd, parserStateBuf, outputData,
				outputDataMaxSize, outputDataLength, packetEnd_s,
				headerInfoBuf);
		//todo: check for errors
	}
	return 0;
}

//TODO: what does this do ?????
void converFromRangedToContentLength(headerInfo_t *hInfo, long fileSize) {
	if (!hInfo->isRange) {
		if (hInfo->transferType == TRANSFER_CHUNKED) {
			hInfo->transferType = TRANSFER_CONTENT_LENGTH;
			hInfo->contentLength = fileSize;
			/* sending the whole file */
		} else {
			printf("it's good the way it is\n\n");
		}
	} else {
		printf("google is sending us a ranged file, this is trouble "
				"because of how we deal with chunking\n");
		hInfo->transferType = TRANSFER_CONTENT_LENGTH;
		hInfo->contentLength = (hInfo->sentContentRangeEnd + 1)
				- hInfo->sentContentRangeStart;
	}
}

//this will download the full header
//send the request to google to get the file
int startFileDownload(char *inputUrl, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		char *extraHeaders) {

	char packetBuffer[MAX_PACKET_SIZE];    //reused quite a bit

	/* request the file from google */
	set_new_header_info(outputHInfo);
	set_new_parser_state_struct(outputParserState);
	printf("we're here now\n");
	/*create the header*/

	utils_createHTTPHeaderFromUrl(inputUrl, packetBuffer, MAX_PACKET_SIZE,
			outputHInfo, REQUEST_GET, extraHeaders);

	utils_connectByUrl(inputUrl, con);
	net_send(con, packetBuffer, strlen(packetBuffer));
	printf("data sent to google %s", packetBuffer);
	return 0;
}

//returns the amount of data downloaded INCLUDING HEADER
int updateFileDownload(Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, char *extraHeaders) {

	int received;
	char packetBuffer[MAX_PACKET_SIZE];
	/* if the parser is already finished return 0*/
	if (outputParserState->currentState == packetEnd_s) {
		return 0;
	}

	/* load the next packet and parse it*/
	received = net_recv(con, packetBuffer, MAX_PACKET_SIZE);

	process_data(packetBuffer, received, outputParserState, outputBuffer,
			outputBufferMaxLength, outputBufferLength, packetEnd_s,
			outputHInfo);

	return received;
}

int finishFileDownload() {
	//close the connection and clean up
	return 0;
}

//the header has already been parsed by this point, hence we need to pass in output data
void downloadDriveFile(AccessTokenState_t *tokenState, Connection_t *clientCon,
		parserState_t *parserState, headerInfo_t *hInfoClientRecv,
		char *outputData, int outputDataLength) {

	int received, chunkSize, outputBufferLength;
	char *url, *size, *accessTokenHeader;
	headerInfo_t hInfoGoogle_response, hInfoClient_response;
	parserState_t parserStateGoogle_response;
	Connection_t con;
	char packetBuffer[MAX_PACKET_SIZE];
	char dataBuffer[MAX_PACKET_SIZE];
	char chunkBuffer[MAX_PACKET_SIZE];

	accessTokenHeader = getAccessTokenHeader(tokenState);
	/* get the url and size of the file using the name given by the url */
	getDownloadUrlAndSize(tokenState, hInfoClientRecv->urlBuffer + 1, &url,
			&size);
	printf("File found, size: %s, url: %s\n", size, url);

	startFileDownload(url, &con, &hInfoGoogle_response,
			&parserStateGoogle_response, accessTokenHeader);

	/* get the header */
	while (!parserStateGoogle_response.headerFullyParsed) {
		updateFileDownload(&con, &hInfoGoogle_response,
				&parserStateGoogle_response, dataBuffer, MAX_PACKET_SIZE,
				&outputBufferLength, accessTokenHeader);
	}

	/* reply to the client about the file */

	//tell the client it's chunked and then send them the header
	hInfoGoogle_response.isRequest = 0;
	hInfoGoogle_response.transferType = TRANSFER_CHUNKED;
	createHTTPHeader(packetBuffer, MAX_PACKET_SIZE, &hInfoGoogle_response,
	NULL);

	net_send(clientCon, packetBuffer, strlen(packetBuffer));

	/* continue downloading and passing data onto the client */

	//FIXME:
	//send any data that might have been in the packets we fetched to get the whole header

	/* keep updating while update doesn't return 0*/
	while (1) {
		received = updateFileDownload(&con, &hInfoGoogle_response,
				&parserStateGoogle_response, dataBuffer, MAX_PACKET_SIZE,
				&outputBufferLength, accessTokenHeader);
		if (received == 0) {
			break;
		}
		//now decrypt that date and send it on to the client (just chunk it)
		flipBits(dataBuffer, received);
		//hm.......we need to decrypt the data here

		chunkSize = utils_chunkData(dataBuffer, outputBufferLength, chunkBuffer);
		if (net_send(clientCon, chunkBuffer, chunkSize) == -1) {
			break;
		}
		outputData[0] = '\0';
	}

	/*clean up everything not covered by finish file download*/
	net_send(clientCon, "0\r\n\r\n", strlen("0\r\n\r\n"));
	finishFileDownload();
	net_close(&con);
}

void handle_client(AccessTokenState_t *stateStruct, int client_fd) {

	/* get the first packet from the client,
	 continue until we have the whole header and see what he wants */

	int outputDataLength;
	char *outputDataBuffer = malloc(MAXDATASIZE + 1);
	parserState_t parserStateClientRecv;
	headerInfo_t hInfoClientRecv;
	Connection_t httpCon;
	net_fileDescriptorToConnection(client_fd, &httpCon);

	getHeader(&httpCon, &parserStateClientRecv, outputDataBuffer,
	MAXDATASIZE, &outputDataLength, &hInfoClientRecv);

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
