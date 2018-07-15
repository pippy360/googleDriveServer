#include "testDownloadDriver.h"


int testDownload_prepDriverForFileTransfer( DriverState_t *driverState ) {

	return 0;
}

int testDownload_downloadInit( FileDownloadState_t *downloadState ) {

	return 0;
}

int testDownload_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength, int *amountOfDataWrittenToBuffer ) {

	return 0;
}

int testDownload_downloadFinish( FileDownloadState_t *downloadState ) {

	return 0;
}

int testDownload_uploadInit( FileUploadState_t *uploadState ) {

	return 0;
}

int testDownload_uploadUpdate( FileUploadState_t *uploadState, const char *inputBuffer,
	int dataLength ) {

	return 0;
}

int testDownload_finishUpload( FileUploadState_t *uploadState ) {

	return 0;
}
