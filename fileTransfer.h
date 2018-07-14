#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "httpProcessing/commonHTTP.h"
#include "httpProcessing/realtimePacketParser.h"
#include "downloadDriver.h"

typedef struct {
	//the amount of the encrypted file downloaded
	//(doesn't include things like http headers,
	//but does include any meta data at the start of the encrypted file like the IV)
	long encryptedDataDownloaded;
	long amountOfFileDecrypted;
	long clientRangeStart;
	long clientRangeEnd;
	long encryptedRangeStart;
	long encryptedRangeEnd;
	CryptoState_t cryptoState;
	char isEndRangeSet;
	char isDecryptionComplete;
} CryptoFileDownloadState_t;

int startFileDownload( FileDownloadState_t *DownloadState );

int updateFileDownload( Connection_t *con, HTTPHeaderState_t *outputHInfo,
		HTTPParserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, 
		const char *extraHeaders );

int finishFileDownload();

int startEncryptedFileDownload( CryptoFileDownloadState_t *encState,
		const char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		HTTPHeaderState_t *outputHInfo, HTTPParserState_t *outputParserState,
		const char *extraHeaders, FileTransferDriver_ops_t *ops, 
		FileDownloadState_t *DownloadState );

int updateEncryptedFileDownload( FileDownloadState_t *downloadState, 
		FileTransferDriver_ops_t *ops, CryptoFileDownloadState_t *encState,
		Connection_t *con, HTTPHeaderState_t *outputHInfo,
		HTTPParserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, 
		const char *extraHeaders );

int finishEncryptedFileDownload( CryptoFileDownloadState_t *encState );


#endif /* FILETRANSFER_H */
