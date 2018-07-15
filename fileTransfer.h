#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "httpProcessing/commonHTTP.h"
#include "httpProcessing/realtimePacketParser.h"
#include "downloadDriver.h"

int startFileDownload( FileDownloadState_t *DownloadState );

int updateFileDownload( FileDownloadState_t *downloadState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength );

int finishFileDownload();

int startEncryptedFileDownload(
		const char *inputUrl, char isRangedRequest, char isEndRangeSet,
		long startRange, long endRange, Connection_t *con,
		HTTPHeaderState_t *outputHInfo, HTTPParserState_t *outputParserState,
		const char *extraHeaders, FileTransferDriver_ops_t *ops, 
		FileDownloadState_t *DownloadState );

int updateEncryptedFileDownload( FileDownloadState_t *downloadState, 
		FileTransferDriver_ops_t *ops,
		Connection_t *con, HTTPHeaderState_t *outputHInfo,
		HTTPParserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, 
		const char *extraHeaders );

int finishEncryptedFileDownload();


#endif /* FILETRANSFER_H */
