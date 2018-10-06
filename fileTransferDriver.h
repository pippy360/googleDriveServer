#ifndef FILETRANSFERDRIVER_H
#define FILETRANSFERDRIVER_H

#include "net/connection.h"
#include "crypto.h"
#include "httpProcessing/commonHTTP.h"
#include "httpProcessing/realtimePacketParser.h"

struct DriverState_t;

typedef struct FileTransferDriver_ops_t FileTransferDriver_ops_t;

//This is state held from the init of the driver
typedef struct DriverState_t {
	FileTransferDriver_ops_t *ops;
	void *priv;
} DriverState_t;

//This is the state held for one file download
typedef struct FileDownloadState_t {
	DriverState_t *driverState;
	CryptoState_t *encryptionState;
	Connection_t *connection;
	HTTPHeaderState_t headerState;
	HTTPParserState_t parserState;
	int isEncrypted;
	int isRangedRequest;
	unsigned long rangeStart;
	unsigned long rangeEnd;
	int isEndRangeSet;
	unsigned long encryptedRangeStart;
	unsigned long encryptedRangeEnd;
	unsigned long encryptedDataDownloaded;
	unsigned long amountOfFileDecrypted;
	int fileDownloadComplete;
	long fileSize;
	char *fileUrl;
	char *requestedFilePath;
	void *priv_connection;
} FileDownloadState_t;

//This is the state held for one file upload
typedef struct FileUploadState_t {
	DriverState_t *driverState;
	CryptoState_t *encryptionState;
	Connection_t *connection;
	HTTPHeaderState_t headerState;
	HTTPParserState_t parserState;
	char *fileUrl;
} FileUploadState_t;


typedef struct FileTransferDriver_ops_t {
	//called once per driver init
	int ( *prepDriverForFileTransfer )( DriverState_t* );

	int ( *downloadInit )( FileDownloadState_t * );
	//returns error code
	int ( *downloadUpdate )( FileDownloadState_t *downloadState, char *outputBuffer,
		int bufferMaxLength );
	//@finishDownload doesn't return any data only cleans up
	int ( *downloadFinish )( FileDownloadState_t * );

	int ( *uploadInit )( FileUploadState_t * );
	int ( *uploadUpdate )( FileUploadState_t *uploadState, const char *inputBuffer,
		int dataLength );
	//@finishUpload doesn't return any data only cleans up
	int ( *finishUpload )( FileUploadState_t * );

} FileTransferDriver_ops_t;

typedef struct PendingFileUpload_t PendingFileUpload_t;
typedef struct PendingFileUpload_t {
        long fileId;
        long currentBytesUploaded;
        PendingFileUpload_t *next;
        void *priv_connection;
} PendingFileUpload_t;

PendingFileUpload_t *getUploadInProgress( long fileId );

int removeUploadInProgress( long fileId );

void init_PendingFileUpload( PendingFileUpload_t *pendingFileUpload, long fileId );

void addUploadInProgress(PendingFileUpload_t *inFileUpload);

#endif /* FILETRANSFERDRIVER_H */
