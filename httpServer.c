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
#include "fileTransfer.h"
#include "crypto.h"
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

//pass in a Connectoin_t, and this will recv until you have the whole header
//it will discard any packet data
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

		printf("packet received from client: --%s--\n\n", packetBuf);

		process_data(packetBuf, recvd, parserStateBuf, outputData,
				outputDataMaxSize, outputDataLength, packetEnd_s,
				headerInfoBuf);
		//TODO: check for errors
	}
	return 0;
}

//FIXME: use a define for the blocksize variable
//the header has already been parsed by this point, hence we need to pass in output data
void downloadDriveFile(AccessTokenState_t *tokenState, Connection_t *clientCon,
		parserState_t *parserState, headerInfo_t *hInfoClientRecv,
		char *outputData, int outputDataLength) {

	CryptoState_t deStateNonPtr;
	CryptoState_t *deState = &deStateNonPtr;
	int received, chunkSize, outputBufferLength;
	char *url, *size, *accessTokenHeaders;
	headerInfo_t hInfoGoogle_response, hInfoClient_response;
	parserState_t parserStateGoogle_response;
	Connection_t con;
	int decryptionOutputLen;
	char packetBuffer[MAX_PACKET_SIZE];
	char dataBuffer[MAX_PACKET_SIZE];
	char chunkBuffer[MAX_PACKET_SIZE];
	char decryptionOutputBuffer[MAX_PACKET_SIZE];
	char isPartial, isRanged;
	int downloadStart, downloadEnd, start, end;
	int blocksize = 16;

	/* get the url and size of the file using the name given by the url */

	getDownloadUrlAndSize(tokenState, hInfoClientRecv->urlBuffer + 1, &url,
			&size);
	printf("File found, size: %s, url: %s\n", size, url);

	accessTokenHeaders = getAccessTokenHeader(tokenState);

	//FIXME: here you'll need to recalc the start and end range to fit the crypto stuff
	//FIXME: REMEMBER the encrypted file will be bigger than the unencrypted one
	//FIXME: then in the response you'll have to make sure the range is set back the correct way

	isPartial = (hInfoClientRecv->getEndRangeSet) ? 0 : 1;

	printf("start is: %lu\n", hInfoClientRecv->getContentRangeStart);

	start = hInfoClientRecv->getContentRangeStart;
	downloadStart = start - (start % blocksize);
	//check if we need an extra block before the start for the IV
	downloadStart = (downloadStart <= 0) ? 0 : downloadStart - blocksize;
	end = hInfoClientRecv->getContentRangeEnd;
	downloadEnd =
			(end % blocksize == 0) ?
					end - 1 : end + (blocksize - (end % blocksize)) - 1;

	startFileDownload(url, hInfoClientRecv->isRange, isPartial, downloadStart,
			downloadEnd, &con, &hInfoGoogle_response,
			&parserStateGoogle_response, accessTokenHeaders);

	int dataDownloadedFromGoogle = 0;//the amount of the FILE (excluding headers) downloaded from google

	/* get the header */
	while (!parserStateGoogle_response.headerFullyParsed) {
		updateFileDownload(&con, &hInfoGoogle_response,
				&parserStateGoogle_response, dataBuffer, MAX_PACKET_SIZE,
				&outputBufferLength, accessTokenHeaders);
		dataDownloadedFromGoogle += outputBufferLength;
	}

	printf("amount of data downloaded after the header: %d\n",
			dataDownloadedFromGoogle);

	while (dataDownloadedFromGoogle < blocksize) {
		updateFileDownload(&con, &hInfoGoogle_response,
				&parserStateGoogle_response,
				dataBuffer + dataDownloadedFromGoogle,
				MAX_PACKET_SIZE - dataDownloadedFromGoogle, &outputBufferLength,
				accessTokenHeaders);
		dataDownloadedFromGoogle += outputBufferLength;
	}

	printf("amount of data downloaded after the second requesting part: %d\n",
			dataDownloadedFromGoogle);



	/* reply to the client about the file */

	//tell the client it's chunked and then send them the header
	printf("\n\n\nok does it fail after this\n\n\n");
	hInfoGoogle_response.isRequest = 0;
	hInfoGoogle_response.transferType = TRANSFER_CHUNKED;
	hInfoGoogle_response.sentContentRangeStart = start;
	if(hInfoClientRecv->getEndRangeSet)
		hInfoGoogle_response.sentContentRangeEnd = hInfoClientRecv->getContentRangeEnd;
	else
		hInfoGoogle_response.sentContentRangeEnd = strtol(size, NULL, 10)-1;
	hInfoGoogle_response.sentContentRangeFull = strtol(size, NULL, 10);
	createHTTPHeader(packetBuffer, MAX_PACKET_SIZE, &hInfoGoogle_response,
			NULL);

	printf("header sent to client: --%s--\n", packetBuffer);

	net_send(clientCon, packetBuffer, strlen(packetBuffer));
	printf("\n\nsome data sent to client --%s--\n\n", packetBuffer);
	/*FIXME: you need to get the first bit of data then ignore it*/

	if (hInfoClientRecv->isRange && downloadStart > 0) {
		startDecryption(deState, "phone", dataBuffer);
	} else {
		printf("no IV needed\n");
		startDecryption(deState, "phone", NULL);
	}
	char *decryptStart;
	if(downloadStart <= 0){
		decryptStart = dataBuffer;
	}else{
		decryptStart = dataBuffer+blocksize;
		outputBufferLength -= blocksize;
	}

	updateDecryption(deState, decryptStart, outputBufferLength,
						decryptionOutputBuffer, &decryptionOutputLen);

	/*now decrypt that date and chunk it and send it*/

	char *startPositionOfData = decryptionOutputBuffer + (start%blocksize);

	chunkSize = utils_chunkData(decryptionOutputBuffer+(start%blocksize), decryptionOutputLen-(start%blocksize),
			chunkBuffer);
	if (net_send(clientCon, chunkBuffer, chunkSize) == -1) {
		printf("sending failed here, 203\n");
	}
	printf("\n\nsome data sent to client 1 --%s--\n\n", chunkBuffer);
	outputData[0] = '\0';
	/* continue downloading and passing data onto the client */

//FIXME: send any data that might have been in the packets we fetched to get the whole header
	/* keep updating while update doesn't return 0*/
	while (1) {
		received = updateFileDownload(&con, &hInfoGoogle_response,
				&parserStateGoogle_response, dataBuffer, MAX_PACKET_SIZE,
				&outputBufferLength, accessTokenHeaders);
		if (received == 0) {
			break;
		}

		/*now decrypt that date and chunk it and send it*/
		updateDecryption(deState, dataBuffer, outputBufferLength,
				decryptionOutputBuffer, &decryptionOutputLen);

		chunkSize = utils_chunkData(decryptionOutputBuffer, decryptionOutputLen,
				chunkBuffer);
		if (net_send(clientCon, chunkBuffer, chunkSize) == -1) {
			break;
		}
		printf("\n\nsome data sent to client 2 --%s--\n\n", chunkBuffer);
		outputData[0] = '\0';
	}
	printf("we've finished the while loop\n");

	/*finish up the decryption*/
	finishDecryption(deState, NULL, 0, decryptionOutputBuffer,
			&decryptionOutputLen);

	printf("we've finished the decryption\n");
	if(decryptionOutputLen > 0){

		chunkSize = utils_chunkData(decryptionOutputBuffer, decryptionOutputLen,
				chunkBuffer);

		printf("we've finished the chunking\n");

		net_send(clientCon, chunkBuffer, chunkSize);
		printf("\n\nsome data sent to client 3 --%s--\n\n", chunkBuffer);

	}
	/*clean up everything not covered by finish file download*/
	net_send(clientCon, "0\r\n\r\n", strlen("0\r\n\r\n"));

	printf("we've finished everything\n");

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
