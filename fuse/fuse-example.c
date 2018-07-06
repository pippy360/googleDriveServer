#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../virtualFileSystem/hiredis/hiredis.h"
#include "../virtualFileSystem/vfs.h"

#include "../google/googleAccessToken.h"
#include "../google/googleUpload.h"

#include "../crypto.h"
#include "../fileTransfer.h"
#include "../utils.h"

#include "../httpProcessing/realtimePacketParser.h"


#define ENCRYPTED_BUFFER_LEN 2000
#define DECRYPTED_BUFFER_LEN 2000
#define STRING_BUFFER_LEN 2000

vfsContext_t ctx;//FIXME: don't use a global 
vfsContext_t *c = &ctx;
AccessTokenState_t accessTokenState;

static int getattr_callback(const char *path, struct stat *stbuf) {
	
	printf("getattr_callback: %s\n", path);

	memset( stbuf, 0, sizeof( struct stat ) );

	vfsPathParserState_t parserState;
	init_vfsPathParserState( &parserState );
 	vfs_parsePath( c, &parserState, path, strlen(path) );

	if ( parserState.fileObj.isDir ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	} else {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = vfs_getFileSize( c, &parserState.fileObj );
		return 0;
	}

	return -ENOENT;
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

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {

	printf("read_callback: %s\n", path);

	//we will under report the size of the buffers to functions when using them as input buffers
	//so that we always meet the size+AES_BLOCK_SIZE requirements of the output buffer for the encrypt/decrypt functions
	//see openssl docs for more info on ecrypted buffer/decrypted buffers size requirements
	char encryptedDataBuffer[ ENCRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char decryptedDataBuffer[ DECRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char strBuf1[ STRING_BUFFER_LEN ]; 
	parserState_t googleParserState;
	CryptoState_t decryptionState;
	vfsPathParserState_t vfsParserState;
	int tempOutputSize;
	Connection_t googleCon;
	headerInfo_t hInfo;

	init_vfsPathParserState( &vfsParserState );
 	vfs_parsePath( c, &vfsParserState, path, strlen(path) );
	vfs_getFileWebUrl( c, &vfsParserState.fileObj, strBuf1, 2000 );

	if (  vfsParserState.isExistingObject && vfsParserState.fileObj.isDir ) {
		long len = vfs_getFileSize( c, &vfsParserState.fileObj );
		if (offset >= len) {
			return 0;	
		}

		startFileDownload( strBuf1, 0, 0, 0, 0, &googleCon, &hInfo,
			&googleParserState, 
			getAccessTokenHeader( &accessTokenState ) );
		long dataCopied = 0;
		int encryptionStarted = 0;
		while ( 1 ) {
			int dataLength;
			int received = updateFileDownload( &googleCon, &hInfo,
					&googleParserState, encryptedDataBuffer,
					ENCRYPTED_BUFFER_LEN, &dataLength, "" );
			if ( !encryptionStarted ) {
				startDecryption( &(decryptionState), "phone", NULL );
				encryptionStarted = 1;
			}
			if ( received == 0 ) {
				//finishDecryption(&decryptionState, tempBuffer, 0, tempBuffer2, &tempBuffer2OutputSize);
				//send(clientState->data_fd, tempBuffer2, tempBuffer2OutputSize, 0);
				break;
			}
			updateDecryption( &decryptionState, encryptedDataBuffer, dataLength,
					decryptedDataBuffer, &tempOutputSize );
			if( tempOutputSize>0 ){
				if ( dataCopied + tempOutputSize > size ) {
					memcpy( buf, decryptedDataBuffer, (size - dataCopied) );
				} else {
					memcpy( buf, decryptedDataBuffer, tempOutputSize);
				}
			}
		}
		finishDecryption( &decryptionState, NULL, 0,
				decryptedDataBuffer, &tempOutputSize );

		return size;
	}

	return -ENOENT;
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
	gat_init_googleAccessToken(&accessTokenState);

	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
