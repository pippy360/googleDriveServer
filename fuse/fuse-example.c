#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../virtualFileSystem/hiredis/hiredis.h"
#include "../virtualFileSystem/vfs.h"

#include "../fileTransferDriver.h"

#include "../google/googleAccessToken.h"
#include "../google/googleUpload.h"

#include "../crypto.h"
#include "../fileTransfer.h"
#include "../utils.h"

#include "../httpProcessing/realtimePacketParser.h"
#include "../google/googleDownloadDriver.h"

#define ENCRYPTED_BUFFER_LEN 2000
#define DECRYPTED_BUFFER_LEN 2000
#define STRING_BUFFER_LEN 2000

#define MAX_PACKET_SIZE 99999


vfsContext_t ctx;//FIXME: don't use a global 
vfsContext_t *c = &ctx;
AccessTokenState_t accessTokenState;
DriverState_t downloadDriver;

static int getattr_callback(const char *path, struct stat *stbuf) {
	
	printf("getattr_callback: %s\n", path);

	memset( stbuf, 0, sizeof( struct stat ) );

	vfsPathParserState_t parserState;
	init_vfsPathParserState( &parserState );
 	vfs_parsePath( c, &parserState, path, strlen(path) );
	if ( !parserState.isExistingObject )
		return -ENOENT;

	if ( parserState.fileObj.isDir ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = vfs_getFileSize( c, &parserState.fileObj );
	}
	return 0;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
  
	printf("readdir_callback: %s\n", path);

	char fuseLsbuf[20000];
	int numRetVals = 0;

	vfsPathParserState_t parserState;
	vfs_parsePath( c, &parserState, path, strlen( path ) );
	vfs_ls( c, &parserState.fileObj, fuseLsbuf, 9999, &numRetVals );

	filler( buf, ".", NULL, 0 );
	filler( buf, "..", NULL, 0 );
	int i;
	char *ptr = fuseLsbuf;
	for ( i = 0; i < numRetVals; i++ ) {
		printf( "\t- %s\n", ptr );
		filler( buf, ptr, NULL, 0 );
		ptr += strlen( ptr ) + 1;
	}

	return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

void initDownloadStateFromFileRead( DriverState_t *driverState, 
		FileDownloadState_t *downloadState, char *fileUrl, long fileSize, 
		size_t downloadSize, 
		off_t offset, int isFileEncrypted ) {

	downloadState->isRangedRequest = 1;
	downloadState->isEndRangeSet = 1;
	downloadState->driverState = driverState;
	downloadState->isEncrypted = 1;
	downloadState->rangeStart = offset;
	downloadState->rangeEnd = offset + downloadSize;
	downloadState->fileSize = fileSize;
	downloadState->fileUrl = malloc( strlen( fileUrl ) );
	memcpy( downloadState->fileUrl, fileUrl, strlen( fileUrl ) );
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	
	char urlBuffer[ 10000 ]; //FIXME: hardcoded value
	vfsPathParserState_t vfsParserState;

	init_vfsPathParserState( &vfsParserState );
 	vfs_parsePath( c, &vfsParserState, path, strlen(path) );
	vfs_getFileWebUrl( c, &vfsParserState.fileObj, urlBuffer, 2000 );

	if ( !vfsParserState.isExistingObject || vfsParserState.fileObj.isDir ) {
		return -ENOENT;
	}


	long len = vfs_getFileSize( c, &vfsParserState.fileObj );
	if (offset >= len) {
		return 0;	
	}

	FileDownloadState_t downloadState;
	int isFileEncrypted = 1;
	initDownloadStateFromFileRead( &downloadDriver, &downloadState, urlBuffer, 
			len, size, offset, isFileEncrypted );
	
	startFileDownload( &downloadState );

	long dataCopied = 0;
	while ( dataCopied < size ) {
		char downloadBuffer[ 10000 ];//FIXME: hardcoded values
		int received = updateFileDownload( &downloadState, downloadBuffer,
				10000 );//FIXME: hardcoded values
		if ( received == 0 ) {
			printf("received == 0 in top loop\n");
			break;
		}
		int dataToCopy = (dataCopied + received > size)? 
				(size - dataCopied) : received ;

		memcpy( buf + dataCopied, downloadBuffer, dataToCopy);
		dataCopied += dataToCopy;
	}
	printf("finished copy----\n rangestart %lu rangeend %lu, encstart %lu encend %lu \n", 
			downloadState.rangeStart,
			downloadState.rangeEnd,
			downloadState.encryptedRangeStart,
			downloadState.encryptedRangeEnd
			 );
	printf("finished copy----\nthe size of this buffer %lu\nThey asked for %lu\n with offset offset %lu\n", dataCopied, size, offset);
//	printf("finished copy---%.*s----\nthe size of this buffer %lu\nThey asked for %lu\n with offset offset %d\n", dataCopied, buf, dataCopied, size, offset);
	return dataCopied;
}

static int create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	printf("create file called with path %s\n", path);
	return -1;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
};

int main(int argc, char *argv[])
{
	if ( vfsContext_init( c ) ) {
		printf("erororrororor\n");
		exit(-1);
	}

	gdrive_prepDriverForFileTransfer( &downloadDriver );

	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
