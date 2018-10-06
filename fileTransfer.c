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
#include "fileTransferDriver.h"

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
		return bufferLength - (decryptedDataFileStart 
				+ amountOfFileDecrypted - clientRangeStart);
	}
	return 0;
}

int __finishFileDownload() {
	//close the connection and clean up
	return 0;
}

int startEncryptedFileDownload( FileDownloadState_t *downloadState ) {
	
	long startRange = downloadState->rangeStart;
	long endRange = downloadState->rangeEnd;

	//calculate the encrypted data we need to request to get the requested decrypted data
	//we will need to fetch the whole of all the encrypted blocks 
	//that the decrytped data overlaps
	long encFileStart, encFileEnd;
	if ( startRange < AES_BLOCK_SIZE ) {
		encFileStart = 0;
	} else {
		encFileStart = startRange - ( startRange % AES_BLOCK_SIZE );
		//if the startRange isn't in the first block then we need the previous block for the IV
		encFileStart -= AES_BLOCK_SIZE;
	}
	if ( downloadState->isEndRangeSet ) {
		if( endRange % AES_BLOCK_SIZE ) {//if not 16 byte aligned
			encFileEnd = endRange + ( AES_BLOCK_SIZE - ( endRange % AES_BLOCK_SIZE ) );
		} else {
			encFileEnd = endRange;
		}
	} else {
		//it doesn't matter what value encFileEnd has
	}

	downloadState->encryptedRangeStart 	= encFileStart;
	downloadState->encryptedRangeEnd 	= encFileEnd;
	downloadState->amountOfFileDecrypted = 0;
	downloadState->encryptedDataDownloaded = 0;
	downloadState->fileDownloadComplete = 0;
	downloadState->encryptionState = malloc( sizeof(CryptoState_t) );//FIXME: this probably isn't the right place for this
	downloadState->connection = malloc( sizeof(Connection_t) );//FIXME: this probably isn't the right place for this

	return downloadState->driverState->ops->downloadInit( downloadState );
}

//FIXME: This code is complicated, test every case
//returns the amount of data in the outputBuffer
int updateEncryptedFileDownload( FileDownloadState_t *downloadState, 
		char *outputBuffer, int outputBufferMaxLength ) {
	printf("in:\t %lu\n", downloadState->encryptedDataDownloaded);
	if( downloadState->fileDownloadComplete ) {
		return 0;
	}

	//if this is the first time update has been called then first fetch 
	//enough data so that we can start the decryption
	int outputBufferLength = 0;	
	if (downloadState->encryptedDataDownloaded == 0) {
		if (downloadState->encryptedRangeStart < AES_BLOCK_SIZE) {
			startDecryption( downloadState->encryptionState, "phone", NULL );
		} else {
			int encryptedPacketLength = 0;
			char encryptedPacketBuffer[MAX_PACKET_SIZE];
			//get the IV
			while (downloadState->encryptedDataDownloaded < AES_BLOCK_SIZE) {
				int result = downloadState->driverState->ops->downloadUpdate(
						downloadState, 
						encryptedPacketBuffer + encryptedPacketLength,
						MAX_PACKET_SIZE );

				if (result == 0) {
					printf("file server closed the connection right at the start");
					//file server closed the connection
					return 0;
				}
				encryptedPacketLength += result;
				downloadState->encryptedDataDownloaded += encryptedPacketLength;
			}
			startDecryption( downloadState->encryptionState, "phone",
					encryptedPacketBuffer );

			//now decrypt any of the extra data we got when getting the IV and add it to the output buffer
			updateDecryption( downloadState->encryptionState,
					encryptedPacketBuffer + AES_BLOCK_SIZE,
					encryptedPacketLength - AES_BLOCK_SIZE, outputBuffer,
					&outputBufferLength );

			downloadState->amountOfFileDecrypted += outputBufferLength;
		}
	} else {
		//printf("not started\n");
	}

	//now that the decryption is started keep fetching data until we have 
	//enough to decrypt at least one byte of data
	if (outputBufferLength == 0) {
		char encryptedPacketBuffer[ MAX_PACKET_SIZE ];	
		int result = downloadState->driverState->ops->downloadUpdate( 
			downloadState, 
			encryptedPacketBuffer, 
			MAX_PACKET_SIZE );
		downloadState->encryptedDataDownloaded += result;
		printf("we got the following result: %d\n", result);
		if ( result == 0 ) {
			printf("result == 0 in bottom loop\n");
			//file server closed the connection
			//check if we reached the end of the http packet
			if( downloadState->parserState.currentState == packetEnd_s ){
				printf("packet end hit\n");
				//check if we have reached the end of the file
				//FIXME: confirm this size is correct
				//FIXME: have these paddings as shared with the orginal padding code 
				long fullfileSize = downloadState->fileSize;
				int startPadding = AES_BLOCK_SIZE;
				int endPadding = (( AES_BLOCK_SIZE - ( fullfileSize % AES_BLOCK_SIZE ) ) % AES_BLOCK_SIZE);
				long endOfEncryptedFile = fullfileSize 
						+ startPadding 
						+ endPadding;
				printf( "range + downloaded\t%lu\n" , downloadState->encryptedRangeStart + downloadState->encryptedDataDownloaded );
				printf( "endOfEncryptedFile\t%lu\n" , endOfEncryptedFile );
				if (downloadState->encryptedRangeStart + downloadState->encryptedDataDownloaded >= endOfEncryptedFile ) {
					// FIXME: log error here
					printf("ERROR: downloadState->encryptedRangeStart + "
						"downloadState->encryptedDataDownloaded > endOfEncryptedFile\n");
				}

				if ( downloadState->encryptedRangeStart + downloadState->encryptedDataDownloaded == 
							endOfEncryptedFile ) {

					printf("and packet ended\n");
					//then finish the encryption
					downloadState->fileDownloadComplete = 1;
					int finalDataLength;
					finishDecryption( downloadState->encryptionState, encryptedPacketBuffer,
							result, outputBuffer,
							&finalDataLength );
					downloadState->amountOfFileDecrypted += finalDataLength;
					outputBufferLength += finalDataLength;
					return outputBufferLength;
				}
			}
			return 0;
		}
		int dataLength;
		updateDecryption( downloadState->encryptionState, encryptedPacketBuffer,
				result, outputBuffer + outputBufferLength,
				&dataLength );
		outputBufferLength += dataLength; //dataLength can be 0, so we might have to loop again
		downloadState->amountOfFileDecrypted += dataLength;
	}

	//remove any extra data from encrypted chunks where we only wanted part 
	//of the chunk 
	long tempStart =
			(downloadState->encryptedRangeStart == 0) ?
					downloadState->encryptedRangeStart :
					downloadState->encryptedRangeStart + 16;//FIXME: magic number
	int trimTopVal;
	if(downloadState->isEndRangeSet){
		trimTopVal = trimTop(downloadState->rangeEnd, tempStart,
				downloadState->amountOfFileDecrypted, outputBufferLength);
	}else{
		trimTopVal = 0;
	}
	int trimBottomVal = trimBottom(downloadState->rangeStart, tempStart,
			downloadState->amountOfFileDecrypted, outputBufferLength);
	outputBufferLength -= trimBottomVal + trimTopVal;
	memmove(outputBuffer, outputBuffer + trimBottomVal, outputBufferLength);
	printf("out:\t %lu\n", downloadState->encryptedDataDownloaded);
	return outputBufferLength;
}

int finishEncryptedFileDownload() {
	return __finishFileDownload();
}

int startEncryptedFileUpload( FileUploadState_t *uploadState, 
		FileTransferDriver_ops_t *ops, const char *inputUrl, 
		Connection_t *con,
		HTTPHeaderState_t *outputHInfo, HTTPParserState_t *outputParserState ) {

	//we will under report the size of the buffers to functions when using them as input buffers
	//so that we always meet the size+AES_BLOCK_SIZE requirements of the output buffer for the encrypt/decrypt functions
	//see openssl docs for more info on ecrypted buffer/decrypted buffers size requirements

	char filenameJson[ STRING_BUFFER_LEN ];
	//get the file name
	printf( "trying to store file %s\n", "FIXME: ADD FILE NAME HERE" );
	sprintf( filenameJson, "\"title\": \"%s\"", "nuffin.bin" );

	ops->uploadInit( uploadState );

	//alright start reading in the file
	startEncryption( uploadState->encryptionState, "phone" );

	return 0;//fixme only one return code
}

//returns the amount of data in the outputBuffer
//this will ONLY return 0 if the connection has been close AND it cannot return any data
int updateEncryptedFileUpload( FileUploadState_t *uploadState, 
		FileTransferDriver_ops_t *ops, Connection_t *con, HTTPHeaderState_t *outputHInfo,
		HTTPParserState_t *outputParserState, const char *inputBuffer,
		int dataLength ) {

/*
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
		ops->uploadUpdate( &googleCon, encryptedDataBuffer, tempOutputSize );

		GoogleUploadState_t fileState;
		googleUpload_end( &googleCon, &fileState );
		vfs_createFile( clientState->ctx, &clientState->ctx->cwd, parserState->paramBuffer,
				storFileSize, fileState.id, fileState.webUrl, fileState.apiUrl );
		sendFtpResponse( clientState, "226 Transfer complete.\r\n" );
	}
	*/
	return 0;//fixme only one return code
}

int finishEncryptedFileUpload() {
	//just clean up and stuff
	return 0;//fixme only one return code
}

int startFileDownload( FileDownloadState_t *downloadState ) {
	return startEncryptedFileDownload( downloadState );
}

//FIXME: move this function to the common code between downloadDrivers
//returns the amount of file data returned in the buffer 
//returns 0 if no more data can be fetched, if this returns 0 then every call after wil return 0 //FIXME: test that this is true 
int updateFileDownload( FileDownloadState_t *downloadState, char *outputBuffer,
		int outputBufferMaxLength ) {

	return updateEncryptedFileDownload( downloadState, outputBuffer, 
		outputBufferMaxLength );
}

int finishFileDownload(FileDownloadState_t *downloadState) {
	return finishEncryptedFileDownload();
}

int startFileUpload( FileUploadState_t *uploadState ) {

	return startEncryptedFileUpload( 
		uploadState, 
		uploadState->driverState->ops, 
		uploadState->fileUrl, 
		uploadState->connection,
		&uploadState->headerState, 
		&uploadState->parserState );
}

//returns the amount of data downloaded INCLUDING HEADER
int updateFileUpload( FileUploadState_t *uploadState, const char *inputBuffer,
		int dataLength ) {

	return updateEncryptedFileUpload(
		uploadState,
		uploadState->driverState->ops, 
		uploadState->connection, 
		&uploadState->headerState,
		&uploadState->parserState, 
		inputBuffer,
		dataLength
		);
}

int finishFileUpload(FileDownloadState_t *uploadState) {
	//close the connection and clean up
	return 0;
}



