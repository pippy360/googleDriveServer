#ifndef GOOGLEDOWNLOADDRIVER_H
#define GOOGLEDOWNLOADDRIVER_H

#include "../downloadDriver.h"

const FileTransferDriver_ops_t gdriveFileTransfer_ops;

int gdrive_prepDriverForFileTransfer( DriverState_t *driverState );

int gdrive_downloadInit( FileDownloadState_t *downloadState );

//returns amountOfDataWrittenToBuffer 
int gdrive_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength);

int gdrive_downloadFinish( FileDownloadState_t *downloadState );

int gdrive_uploadInit( FileUploadState_t *uploadState );

int gdrive_uploadUpdate( FileUploadState_t *uploadState, const char *inputBuffer,
	int dataLength );

int gdrive_finishUpload( FileUploadState_t *uploadState );


#endif /* GOOGLEDOWNLOADDRIVER_H */