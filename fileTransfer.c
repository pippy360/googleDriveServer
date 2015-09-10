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

int startEncryptedFileDownload(CryptoFileDownloadState_t *encState,
		char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		char *extraHeaders) {

	/*calculate the encrypted data we need to request to get the requested decrypted data*/
	long encFileStart, encFileEnd;

	encState->isDecryptionComplete = 0;
	encState->isEndRangeSet = isEndRangeSet;

	if (startRange < AES_BLOCK_SIZE) {
		encFileStart = 0;
	} else {
		encFileStart = startRange - (startRange % AES_BLOCK_SIZE);
		/*if the startRange isn't in the first block then we need the previous block for the IV*/
		encFileStart -= AES_BLOCK_SIZE;
	}
	if (isEndRangeSet) {
		encFileEnd = endRange + (AES_BLOCK_SIZE - (endRange % AES_BLOCK_SIZE));
	} else {
		//it doesn't matter what value encFileEnd has
	}

	encState->clientRangeStart = startRange;
	encState->clientRangeEnd = endRange;
	encState->encryptedRangeStart = encFileStart;
	encState->encryptedRangeEnd = encFileEnd;
	encState->encryptedDataDownloaded = 0;

	return startFileDownload(inputUrl, isRangedRequest, isEndRangeSet,
			encFileStart, encFileEnd, con, outputHInfo, outputParserState,
			extraHeaders);
}

//returns the amount of data in the outputBuffer
//this will ONLY return 0 if the connection has been close AND it cannot return any data
//FIXME:  THERE'S NO NEED TO RETURN THE OUTPUTBUFFERLENGTH, IT'S ONLY CONFUSING
int updateEncryptedFileDownload(CryptoFileDownloadState_t *encState,
		Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, char *extraHeaders) {

	*outputBufferLength = 0; //the amount of data downloaded during this function call (including header)
	int result; //use to store the return of updateFileDownload
	char encryptedPacketBuffer[MAX_PACKET_SIZE];
	int encryptedPacketLength = 0;
	int dataLength;

	//printf("about to handle the start stuff\n");
	/*if this is the first time update has been called then call startDecryption*/
	if (encState->encryptedDataDownloaded == 0) {
		if (encState->encryptedRangeStart < AES_BLOCK_SIZE) {
			//printf("the start range was 0, here it is: %lu\n",
				//	encState->encryptedRangeStart);
			startDecryption(&(encState->cryptoState), "phone", NULL);
		} else {
//			printf("the start range was NOT 0, here it is: %lu\n",
	//				encState->encryptedRangeStart);
			/*get the IV*/
			while (encState->encryptedDataDownloaded < AES_BLOCK_SIZE) {
				result = updateFileDownload(con, outputHInfo, outputParserState,
						encryptedPacketBuffer
								+ encState->encryptedDataDownloaded,
						MAX_PACKET_SIZE - encState->encryptedDataDownloaded,
						&encryptedPacketLength, extraHeaders);
				if (result == 0) {
					/*file server closed the connection*/
					return *outputBufferLength;
				}
				encState->encryptedDataDownloaded += encryptedPacketLength;
			}
			startDecryption(&(encState->cryptoState), "phone",
					encryptedPacketBuffer);

			/*now decrypt any of the extra data we got when getting the IV and add it to the output buffer*/
			updateDecryption(&(encState->cryptoState),
					encryptedPacketBuffer + AES_BLOCK_SIZE,
					encryptedPacketLength - AES_BLOCK_SIZE, outputBuffer,
					outputBufferLength);
			encState->amountOfFileDecrypted += *outputBufferLength;
		//	printf(
			//		"so we just decrypted the data we got with the IV, lets look at it\n");
			//printf("here: --%.*s--\n", *outputBufferLength, outputBuffer);
		}
	} else {
		//printf("not started\n");
	}
//	printf("finished with the start stuff\n");

	/*make sure we have some decrypted data to return*/
	while (*outputBufferLength == 0) {
		//if it fails updateDOWN
		result = updateFileDownload(con, outputHInfo, outputParserState,
				encryptedPacketBuffer, MAX_PACKET_SIZE, &encryptedPacketLength,
				extraHeaders);
		encState->encryptedDataDownloaded += encryptedPacketLength;
		if (result == 0) {
			/*file server closed the connection*/
			//check if we reached the end of the http packet
			if(outputParserState->currentState == packetEnd_s && !encState->isDecryptionComplete){
				//then finish the encryption
				encState->isDecryptionComplete = 1;
				finishDecryption(&(encState->cryptoState), encryptedPacketBuffer,
						encryptedPacketLength, outputBuffer,
						&dataLength);
				*outputBufferLength += dataLength;
				return *outputBufferLength;
			}
			return 0;
		}
//		printf(
	//			"ok we've recv'd, now lets decrypt, recv'd: %d sent to decrypt: %d\n",
		//		result, encryptedPacketLength);
		updateDecryption(&(encState->cryptoState), encryptedPacketBuffer,
				encryptedPacketLength, outputBuffer + (*outputBufferLength),
				&dataLength);
		(*outputBufferLength) += dataLength; //dataLength can be 0, so we might have to loop again
		encState->amountOfFileDecrypted += dataLength;
	}

	//write the trim function
	//TODO: IMPROVE EXPLINATION
	//remove extra data. We can have extra data because the data must decypted in chunks
	long tempStart =
			(encState->encryptedRangeStart == 0) ?
					encState->encryptedRangeStart :
					encState->encryptedRangeStart + 16;
	int trimTopVal;
	if(encState->isEndRangeSet){
		trimTopVal = trimTop(encState->clientRangeEnd, tempStart,
				encState->amountOfFileDecrypted, *outputBufferLength);
	}else{
		trimTopVal = 0;
	}
	int trimBottomVal = trimBottom(encState->clientRangeStart, tempStart,
			encState->amountOfFileDecrypted, *outputBufferLength);
	//printf("trim bottom %d, trim top %d\n", trimBottomVal, trimTopVal);
//	printf("before memcpy --%.*s--\n", (*outputBufferLength), outputBuffer);
	(*outputBufferLength) -= trimBottomVal + trimTopVal;
	memmove(outputBuffer, outputBuffer + trimBottomVal, *outputBufferLength);
	//printf("after memcpy %d --%.*s--\n", *outputBufferLength,
		//	(*outputBufferLength), outputBuffer);

	return *outputBufferLength;
}

//returns the amount that needs to be trimmed off the top
//amountOfFileDecrypted must be kept up to date
int trimTop(long clientRangeEnd, long decryptedDataFileStart,
		long amountOfFileDecrypted, int bufferLength) {
	//the RangeEnd is inclusive so you must -1 here
	if ((decryptedDataFileStart + amountOfFileDecrypted - 1) > clientRangeEnd) {
		//then trim
		return ((decryptedDataFileStart + amountOfFileDecrypted - 1)
				- clientRangeEnd);
	}
	return 0;
}

//returns the amount that needs to be trimmed off the bottom
int trimBottom(long clientRangeStart, long decryptedDataFileStart,
		long amountOfFileDecrypted, int bufferLength) {

	//printf("trim bottom called\n");
	int temp;
	//printf("if test calc: %lu clientRange: %lu\n",
		//	decryptedDataFileStart + amountOfFileDecrypted - bufferLength,
			//clientRangeStart);
	//printf(
		//	"bufferLength: %d\nencryptedRangeStart: %lu\namountOfFileDecrypted: %lu\n",
			//bufferLength, decryptedDataFileStart, amountOfFileDecrypted);
	if ((decryptedDataFileStart + amountOfFileDecrypted - bufferLength)
			< clientRangeStart) {
//		printf("needs trimming!\n");
		return bufferLength
				- (decryptedDataFileStart + amountOfFileDecrypted
						- clientRangeStart);
	}
	return 0;
}

void finishEncryptedFileDownload(CryptoFileDownloadState_t *encState) {
	finishFileDownload();
}

//this will download the full header
//send the request to google to get the file
//FIXME: start file download might not always return data,
//FIXME: for example if you only get one byte at a time and the one byte is part of the chunking stuff
//FIXME: this isn't taken into account in the other parts of the code, you might end up sending 0 chunks early
int startFileDownload(char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		char *extraHeaders) {

	char packetBuffer[MAX_PACKET_SIZE];    //reused quite a bit

	/* request the file from google */
	set_new_header_info(outputHInfo);
	set_new_parser_state_struct(outputParserState);

	/*create the header*/
	utils_setHInfoFromUrl(inputUrl, outputHInfo, REQUEST_GET, extraHeaders);

	outputHInfo->isRange = isRangedRequest;
	outputHInfo->getContentRangeStart = startRange;
	outputHInfo->getContentRangeEnd = endRange;
	outputHInfo->getEndRangeSet = isEndRangeSet;
	createHTTPHeader(packetBuffer, MAX_PACKET_SIZE, outputHInfo, extraHeaders);

	utils_connectByUrl(inputUrl, con);
	net_send(con, packetBuffer, strlen(packetBuffer));
	printf("data sent to google --%s--\n\n", packetBuffer);

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

	if (!outputParserState->headerFullyParsed)
		printf("updateFileDownload called, this is the data --%s--\n",
				packetBuffer);

	process_data(packetBuffer, received, outputParserState, outputBuffer,
			outputBufferMaxLength, outputBufferLength, packetEnd_s,
			outputHInfo);
	return received;
}

int finishFileDownload() {
	//close the connection and clean up
	return 0;
}
