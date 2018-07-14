#ifndef GOOGLEDOWNLOADDRIVER_H
#define GOOGLEDOWNLOADDRIVER_H

#include "../downloadDriver.h"

const FileTransferDriver_ops_t googleDriveFileTransfer_ops;


int gdrive_prepDriverForFileTransfer( DriverState_t *driverState );

int gdrive_downloadInit( FileDownloadState_t * );

int gdrive_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength, int *amountOfDataWrittenToBuffer );

int gdrive_downloadFinish( FileDownloadState_t * );

int gdrive_uploadInit( FileUploadState_t * );

int gdrive_uploadUpdate( FileUploadState_t *uploadState, const char *inputBuffer,
	int dataLength );

int gdrive_finishUpload( FileUploadState_t * );


typedef struct {

	.prepDriverForFileTransfer = gdrive_prepDriverForFileTransfer;

	.downloadInit 	= gdrive_downloadInit;

	.downloadUpdate = gdrive_downloadUpdate;

	.downloadFinish = gdrive_downloadFinish;

	.uploadInit 	= gdrive_uploadInit;

	.uploadUpdate 	= gdrive_uploadUpdate;

	.finishUpload 	= gdrive_finishUpload;

} googleDriveFileTransfer_ops;


#endif /* GOOGLEDOWNLOADDRIVER_H */