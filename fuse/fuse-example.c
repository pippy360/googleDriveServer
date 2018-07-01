#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../virtualFileSystem/hiredis/hiredis.h"
#include "../virtualFileSystem/vfs.h"

#include "google/googleAccessToken.h"
#include "google/googleUpload.h"

#include "crypto.h"
#include "fileTransfer.h"
#include "utils.h"

static const char *filepath = "/file";
static const char *filecontent = "I'm the content of the only file available there\n";
vfsContext_t ctx;//FIXME: don't use a global 
vfsContext_t *c = &ctx;
AccessTokenState_t stateStruct;

static int getattr_callback(const char *path, struct stat *stbuf) {
	
	printf("2the path they asked for: %s\n", path);

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
		stbuf->st_size = vfs_getFileSize( c, parserState->fileObj );
		return 0;
	}

	return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
  
	printf("the path they asked for: %s\n", path);

	char fuseLsbuf[20000];
	int numRetVals = 0;

	vfsPathParserState_t parserState;
	vfs_parsePath( c, &parserState, path, strlen( path ) );
	vfs_ls( c, &parserState.fileObj, fuseLsbuf, 9999, &numRetVals );

	int i;
	char *ptr = fuseLsbuf;
	for ( i = 0; i < numRetVals; i++ ) {
		filler( buf, ptr, NULL, 0 );
		ptr += strlen( ptr );
	}

	return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {

	printf("3the path they asked for: %s\n", path);

	//we will under report the size of the buffers to functions when using them as input buffers
	//so that we always meet the size+AES_BLOCK_SIZE requirements of the output buffer for the encrypt/decrypt functions
	//see openssl docs for more info on ecrypted buffer/decrypted buffers size requirements
	char encryptedDataBuffer[ ENCRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char decryptedDataBuffer[ DECRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char strBuf1[ STRING_BUFFER_LEN ], strBuf2[ STRING_BUFFER_LEN ];
	parserState_t googleParserState;
	CryptoState_t decryptionState;
	vfsPathParserState_t vfsParserState;
	int tempOutputSize;

	vfsPathParserState_t vfsParserState;
	init_vfsPathParserState( &vfsParserState );
 	vfs_parsePath( c, &vfsParserState, path, strlen(path) );
	vfs_getFileWebUrl( clientState->ctx, &vfsParserState.fileObj, strBuf1, 2000 );

	if (  vfsParserState.isExistingObject && vfsParserState.fileObj.isDir ) {
		long len = vfs_getFileSize( c, vfsParserState->fileObj );
		if (offset >= len) {
			return 0;	
		}

		startFileDownload( strBuf1, 0, 0, 0, 0, &googleCon, &hInfo,
			&googleParserState, 
			getAccessTokenHeader( accessTokenState ) );

		if (offset + size > len) {
			//load the file
			memcpy(buf, filecontent + offset, len - offset);
			return len - offset;
		}

		memcpy(buf, filecontent + offset, size);

		return size;
	}
	return -ENOENT;


	//we will under report the size of the buffers to functions when using them as input buffers
	//so that we always meet the size+AES_BLOCK_SIZE requirements of the output buffer for the encrypt/decrypt functions
	//see openssl docs for more info on ecrypted buffer/decrypted buffers size requirements
	char encryptedDataBuffer[ ENCRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char decryptedDataBuffer[ DECRYPTED_BUFFER_LEN + AES_BLOCK_SIZE ];
	char strBuf1[ STRING_BUFFER_LEN ], strBuf2[ STRING_BUFFER_LEN ];
	parserState_t googleParserState;
	CryptoState_t decryptionState;
	vfsPathParserState_t vfsParserState;
	int tempOutputSize;

	//FIXME: make sure we have a connection open
	//send a get request to the and then continue the download
	//take the download out to it's own file
	vfs_parsePath( clientState->ctx, &vfsParserState, parserState->paramBuffer,
			strlen(parserState->paramBuffer) );

	//FIXME: handle if the file doesn't exist or if it's not a valid path
	if( !vfsParserState.isFilePath ){
		sendFtpResponse( clientState, "500 bad parameters, it's either not a file or it doesn't exist!\r\n" );
		break;
	}
	vfs_getFileWebUrl( clientState->ctx, &vfsParserState.fileObj, strBuf1, 2000 );
	printf( "the url of the file they're looking for is: %s\n", strBuf1 );

	startFileDownload( strBuf1, 0, 0, 0, 0, &googleCon, &hInfo,
			&googleParserState, 
			getAccessTokenHeader( accessTokenState ) );
	sendFtpResponse(clientState, "150 about to send file\r\n");
	encryptionStarted = 0;
	while ( 1 ) {
		received = updateFileDownload( &googleCon, &hInfo,
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
			send( clientState->data_fd, decryptedDataBuffer, tempOutputSize, 0 );
		}
	}
	finishDecryption( &decryptionState, NULL, 0,
			decryptedDataBuffer, &tempOutputSize );
	if( tempOutputSize > 0 ){
		send( clientState->data_fd, decryptedDataBuffer, tempOutputSize, 0 );
	}
	if ( close(clientState->data_fd2) != 0 ) {
		perror( "close:" );
	}
	if ( close(clientState->data_fd) != 0 ) {
		perror( "close:" );
	}
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
	gat_init_googleAccessToken(&stateStruct);

	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
