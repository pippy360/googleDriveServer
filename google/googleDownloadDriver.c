#include "googleDownloadDriver.h"


int gdrive_prepDriverForFileTransfer( DriverState_t *driverState ) {

	return 0;
}

int gdrive_downloadInit( FileDownloadState_t *downloadState ) {

	return 0;
}

int gdrive_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength, int *amountOfDataWrittenToBuffer ) {

	return 0;
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
