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
