#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "testDownloadDriver.h"

#define BUF_SIZE 1000

typedef struct Priv_fd_t {
	int fd;
} Priv_fd_t;

const FileTransferDriver_ops_t testDownloadFileTransfer_ops = {

        .prepDriverForFileTransfer = testDownload_prepDriverForFileTransfer,

        .downloadInit   = testDownload_downloadInit,

        .downloadUpdate = testDownload_downloadUpdate,

        .downloadFinish = testDownload_downloadFinish,

        .uploadInit     = testDownload_uploadInit,

        .uploadUpdate   = testDownload_uploadUpdate,

        .finishUpload   = testDownload_finishUpload,

};


int testDownload_prepDriverForFileTransfer( DriverState_t *driverState ) {
	driverState->ops = &testDownloadFileTransfer_ops;
	return 0;
}

int testDownload_downloadInit( FileDownloadState_t *downloadState ) {
	char strbuf[ 10000 ] = "./";
	printf( "%s\n", downloadState->requestedFilePath );
	char *strbufptr = strcat( strbuf, downloadState->requestedFilePath );
	printf( "testDownloadDriver: reading this file (real path) ----%s----\n", strbufptr );
	downloadState->priv_connection = malloc( sizeof( Priv_fd_t ) );
	Priv_fd_t *fileCon = downloadState->priv_connection;
	fileCon->fd = open( strbufptr, O_RDONLY );
	if ( fileCon->fd == -1 ) {
		printf( "testdownloaddriver failed to open the file\n" );
		return -1;
	}
	return 0;
}

int testDownload_downloadUpdate( FileDownloadState_t *downloadState, char *outputBuffer,
	int bufferMaxLength ) {

	Priv_fd_t *fileCon = downloadState->priv_connection;
	ssize_t ret_in = read ( fileCon->fd, outputBuffer, bufferMaxLength );
	return ret_in;
}

int testDownload_downloadFinish( FileDownloadState_t *downloadState ) {
	Priv_fd_t *fileCon = downloadState->priv_connection;
	close( fileCon->fd );
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
