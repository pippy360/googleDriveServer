#include "googleDownloadDriver.h"
#include "googleAccessToken.h"
#include "../utils.h"
#include "../httpProcessing/createHTTPHeader.h"
#include "../net/connection.h"

#define MAX_PACKET_SIZE 100000

FileTransferDriver_ops_t gdriveFileTransfer_ops = {

	.prepDriverForFileTransfer = gdrive_prepDriverForFileTransfer,

	.downloadInit 	= gdrive_downloadInit,

	.downloadUpdate = gdrive_downloadUpdate,

	.downloadFinish = gdrive_downloadFinish,

	.uploadInit 	= gdrive_uploadInit,

	.uploadUpdate 	= gdrive_uploadUpdate,

	.finishUpload 	= gdrive_finishUpload,

};

int gdrive_prepDriverForFileTransfer( DriverState_t *driverState ) {
	AccessTokenState_t *stateStruct = malloc( sizeof( AccessTokenState_t ) );
	driverState->priv = (void *) stateStruct;
	driverState->ops = &gdriveFileTransfer_ops;
	return gat_init_googleAccessToken( stateStruct );
}

//this will send the request to google to get the file 
//and download the full http response header
int gdrive_downloadInit( FileDownloadState_t *downloadState ) {

	char packetBuffer[ MAX_PACKET_SIZE ];    //reused quite a bit
	HTTPHeaderState_t *outputHInfo = &downloadState->headerState;
	HTTPParserState_t *outputParserState = &downloadState->parserState;

	set_new_header_info( outputHInfo );
	set_new_parser_state_struct( outputParserState );

	//create the header
	char *extraHeaders = utils_shittyGetAccessTokenHeader( 
			(AccessTokenState_t *) downloadState->driverState->priv );

	utils_setHInfoFromUrl( downloadState->fileUrl, outputHInfo, 
			REQUEST_GET, extraHeaders );
	
	outputHInfo->isRange = downloadState->isRangedRequest;
	outputHInfo->getContentRangeStart = downloadState->rangeStart;
	outputHInfo->getContentRangeEnd = downloadState->rangeEnd;
	outputHInfo->getEndRangeSet = downloadState->isEndRangeSet;

	createHTTPHeader( packetBuffer, MAX_PACKET_SIZE, outputHInfo, 
			extraHeaders );

	//init connection
	downloadState->connection = malloc( sizeof(Connection_t) );//FIXME: this probably isn't the right place for this
	downloadState->encryptionState = malloc( sizeof(CryptoState_t) );//FIXME: this probably isn't the right place for this
	utils_connectByUrl( downloadState->fileUrl, downloadState->connection );
	//request the file from google
	net_send( downloadState->connection, packetBuffer, strlen( packetBuffer ) );

	return 0;
}

int gdrive_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength ) {

	int received;
	char packetBuffer[ MAX_PACKET_SIZE ];

	//if the parser is already finished return 0
	if ( downloadState->parserState.currentState == packetEnd_s ) {
		return 0;
	}

	//load the next packet and parse it
	received = net_recv( downloadState->connection, packetBuffer, 
			MAX_PACKET_SIZE );

	//FIXME: handle errors
	int amountOfDataWrittenToBuffer;
	process_data( packetBuffer, received, &downloadState->parserState, outputBuffer,
			bufferMaxLength, &amountOfDataWrittenToBuffer, packetEnd_s,
			&downloadState->headerState );
	return amountOfDataWrittenToBuffer;
}

int gdrive_downloadFinish( FileDownloadState_t *downloadState ) {

	return 0;
}

int gdrive_uploadInit( FileUploadState_t *uploadState ) {

	return 0;
}

int gdrive_uploadUpdate( FileUploadState_t *uploadState, const char *inputBuffer,
	int dataLength ) {

	return 0;
}

int gdrive_finishUpload( FileUploadState_t *uploadState ) {

	return 0;
}
