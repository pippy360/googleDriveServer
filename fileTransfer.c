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
#include "utils.h"
#include "crypto.h"
#include "fileTransfer.h"
#include "downloadDriver.h"

#define ENCRYPTED_BUFFER_LEN 10000
#define DECRYPTED_BUFFER_LEN 10000
#define STRING_BUFFER_LEN 10000

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

	if ((decryptedDataFileStart + amountOfFileDecrypted - bufferLength)
			< clientRangeStart) {
		return bufferLength
				- (decryptedDataFileStart + amountOfFileDecrypted
						- clientRangeStart);
	}
	return 0;
}

//this will download the full header
//send the request to google to get the file
//FIXME: start file download might not always return data,
//FIXME: for example if you only get one byte at a time and the one byte is part of the chunking stuff
//FIXME: this isn't taken into account in the other parts of the code, you might end up sending 0 chunks early
int __startFileDownload(const char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		const char *extraHeaders) {

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

	return 0;
}

//returns the amount of data downloaded INCLUDING HEADER
int __updateFileDownload( Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, 
		const char *extraHeaders ) {

	int received;
	char packetBuffer[ MAX_PACKET_SIZE ];
	/* if the parser is already finished return 0*/
	if ( outputParserState->currentState == packetEnd_s ) {
		return 0;
	}

	/* load the next packet and parse it*/
	received = net_recv( con, packetBuffer, MAX_PACKET_SIZE );

	process_data( packetBuffer, received, outputParserState, outputBuffer,
			outputBufferMaxLength, outputBufferLength, packetEnd_s,
			outputHInfo );
	return received;
}

int __finishFileDownload() {
	//close the connection and clean up
	return 0;
}

//@encState takes in a new CryptoFileDownloadState_t which will be cleared
int startEncryptedFileDownload(CryptoFileDownloadState_t *encState,
		const char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		const char *extraHeaders) {

	/*calculate the encrypted data we need to request to get the requested decrypted data*/
	long encFileStart, encFileEnd;

	//FIXME: clear encState

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

	return __startFileDownload(inputUrl, isRangedRequest, isEndRangeSet,
			encFileStart, encFileEnd, con, outputHInfo, outputParserState,
			extraHeaders);
}

//returns the amount of data in the outputBuffer
//this will ONLY return 0 if the connection has been close AND it cannot return any data
//FIXME:  THERE'S NO NEED TO RETURN THE OUTPUTBUFFERLENGTH, IT'S ONLY CONFUSING
int updateEncryptedFileDownload( CryptoFileDownloadState_t *encState,
		Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, 
		const char *extraHeaders ) {

	*outputBufferLength = 0; //the amount of data downloaded during this function call (including header)
	int result; //use to store the return of updateFileDownload
	char encryptedPacketBuffer[MAX_PACKET_SIZE];
	int encryptedPacketLength = 0;
	int dataLength;

	/*if this is the first time update has been called then call startDecryption*/
	if (encState->encryptedDataDownloaded == 0) {
		if (encState->encryptedRangeStart < AES_BLOCK_SIZE) {
			startDecryption(&(encState->cryptoState), "phone", NULL);
		} else {
			/*get the IV*/
			while (encState->encryptedDataDownloaded < AES_BLOCK_SIZE) {
				result = __updateFileDownload(con, outputHInfo, outputParserState,
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
		}
	} else {
		//printf("not started\n");
	}

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
	(*outputBufferLength) -= trimBottomVal + trimTopVal;
	memmove(outputBuffer, outputBuffer + trimBottomVal, *outputBufferLength);

	return *outputBufferLength;
}

int finishEncryptedFileDownload(CryptoFileDownloadState_t *encState) {
	return __finishFileDownload();
}

int startEncryptedFileUpload( CryptoFileDownloadState_t *encState,
		const char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		const char *extraHeaders ) {

	//we will under report the size of the buffers to functions when using them as input buffers
	//so that we always meet the size+AES_BLOCK_SIZE requirements of the output buffer for the encrypt/decrypt functions
	//see openssl docs for more info on ecrypted buffer/decrypted buffers size requirements
	CryptoState_t encryptionState;
	int tempOutputSize;
	Connection_t googleCon;
	headerInfo_t hInfo;

	char filenameJson[ STRING_BUFFER_LEN ];
	//get the file name
	printf( "trying to store file %s\n", "FIXME: ADD FILE NAME HERE" );
	sprintf( filenameJson, "\"title\": \"%s\"", "nuffin.bin" );
	googleUpload_init( &googleCon, accessTokenState, filenameJson, 
		"image/png" );

	//alright start reading in the file
	startEncryption( &encryptionState, "phone" );

	return 0;//fixme only one return code
}

//returns the amount of data in the outputBuffer
//this will ONLY return 0 if the connection has been close AND it cannot return any data
//FIXME:  THERE'S NO NEED TO RETURN THE OUTPUTBUFFERLENGTH, IT'S ONLY CONFUSING
int updateEncryptedFileUpload( CryptoFileDownloadState_t *encState,
		Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, 
		const char *extraHeaders ) {

	long received;
	long storFileSize = 0;
	char encryptedDataBuffer[ ENCRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char decryptedDataBuffer[ DECRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];

	if ( ( received = recv( clientState->data_fd, decryptedDataBuffer,
		DECRYPTED_BUFFER_LEN, 0 ) ) > 0) {

		storFileSize += received;
		updateEncryption( &encryptionState, decryptedDataBuffer, received,
				encryptedDataBuffer, &tempOutputSize );
		googleUpload_update( &googleCon, encryptedDataBuffer,
				tempOutputSize );

	} else {
		//call this one final time
		finishEncryption( &encryptionState, decryptedDataBuffer, received,
				encryptedDataBuffer, &tempOutputSize );
		googleUpload_update( &googleCon, encryptedDataBuffer, tempOutputSize );

		GoogleUploadState_t fileState;
		googleUpload_end( &googleCon, &fileState );
		vfs_createFile( clientState->ctx, &clientState->ctx->cwd, parserState->paramBuffer,
				storFileSize, fileState.id, fileState.webUrl, fileState.apiUrl );
		sendFtpResponse( clientState, "226 Transfer complete.\r\n" );
	}
	return 0;//fixme only one return code
}

void finishEncryptedFileUpload( CryptoFileDownloadState_t *encState ) {
	//just clean up and stuff
	return 0;//fixme only one return code
}

void getFileDownloadStateFromFileRead( const char *inputUrl, 
		char isEncrypted, long offset, long size, FileDownload_t *downloadState ) {

	downloadState->isEncrypted = isEncrypted;
	downloadState->rangeStart = offset;
	downloadState->rangeEnd = offset + size;
	downloadState->connection = con;
	downloadState->encryptionState = CryptoFileDownloadState_t;
}

int startFileDownload( FileDownload_t *downloadState ) {

	return startEncryptedFileDownload( downloadState->encryptionState,
		downloadState->fileUrl, 1 /* isRangedRequest */, 1 /* isEndRangeSet */,
		downloadState->rangeStart, downloadState->rangeEnd, downloadState->connection,
		&downloadState->outputHInfo, &downloadState->parserState,
		&downloadState->getExtraHeaders() );
	return 0;//fixme only one return code
}

//returns the amount of data downloaded INCLUDING HEADER
int updateFileDownload( FileDownload_t *downloadState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength ) {

	return updateEncryptedFileDownload( downloadState->encryptionState,
		downloadState->connection, &downloadState->outputHInfo, 
		&downloadState->parserState, outputBuffer,
		outputBufferMaxLength, outputBufferLength, 
		&downloadState->getExtraHeaders() );
}

int finishFileDownload(FileDownload_t *downloadState) {
	return finishEncryptedFileDownload( downloadState->encryptionState );
}

int startFileUpload( FileUpload_t *uploadState ) {

	return startEncryptedFileUpload( uploadState->encryptionState,
		uploadState->fileUrl, 1 /* isRangedRequest */, 1 /* isEndRangeSet */,
		uploadState->rangeStart, uploadState->rangeEnd, uploadState->connection,
		&uploadState->outputHInfo, &uploadState->parserState,
		&uploadState->getExtraHeaders() );
}

//returns the amount of data downloaded INCLUDING HEADER
int updateFileUpload( FileDownload_t *uploadState, const char *inputBuffer,
		int inputBufferMaxLength, int *inputBufferLength ) {

	return updateEncryptedFileUpload( uploadState->encryptionState,
		uploadState->connection, uploadState->encryptionState,
		uploadState->parserState, inputBuffer,
		inputBufferMaxLength, inputBufferLength, 
		&uploadState->getExtraHeaders() );
}

int finishFileUpload(FileDownload_t *uploadState) {
	//close the connection and clean up
	return 0;
}



