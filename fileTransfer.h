#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "httpProcessing/commonHTTP.h"
#include "httpProcessing/realtimePacketParser.h"
#include "downloadDriver.h"

int startFileDownload( FileDownloadState_t *DownloadState );

int updateFileDownload( FileDownloadState_t *downloadState, char *outputBuffer,
		int outputBufferMaxLength );

int finishFileDownload();

int startEncryptedFileDownload(	FileDownloadState_t *DownloadState );

int updateEncryptedFileDownload( FileDownloadState_t *downloadState, 
		char *outputBuffer, int outputBufferMaxLength );

int finishEncryptedFileDownload();


#endif /* FILETRANSFER_H */
