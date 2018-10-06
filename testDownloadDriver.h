#ifndef TESTDOWNLOADDRIVER_H
#define TESTDOWNLOADDRIVER_H

#include "../fileTransferDriver.h"

const FileTransferDriver_ops_t testDownloadFileTransfer_ops;


int testDownload_prepDriverForFileTransfer( DriverState_t *driverState );

int testDownload_downloadInit( FileDownloadState_t *downloadState );

int testDownload_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength, int *amountOfDataWrittenToBuffer );

int testDownload_downloadFinish( FileDownloadState_t *downloadState );

int testDownload_uploadInit( FileUploadState_t *uploadState );

int testDownload_uploadUpdate( FileUploadState_t *uploadState, const char *inputBuffer,
	int dataLength );

int testDownload_finishUpload( FileUploadState_t *uploadState );


const FileTransferDriver_ops_t testDownloadFileTransfer_ops = {

	.prepDriverForFileTransfer = testDownload_prepDriverForFileTransfer,

	.downloadInit 	= testDownload_downloadInit,

	.downloadUpdate = testDownload_downloadUpdate,

	.downloadFinish = testDownload_downloadFinish,

	.uploadInit 	= testDownload_uploadInit,

	.uploadUpdate 	= testDownload_uploadUpdate,

	.finishUpload 	= testDownload_finishUpload,

};


#endif /* TESTDOWNLOADDRIVER_H */
