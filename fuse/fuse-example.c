#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../virtualFileSystem/hiredis/hiredis.h"
#include "../virtualFileSystem/vfs.h"

#include "../downloadDriver.h"

#include "../google/googleAccessToken.h"
#include "../google/googleUpload.h"

#include "../crypto.h"
#include "../fileTransfer.h"
#include "../utils.h"

#include "../httpProcessing/realtimePacketParser.h"


#define ENCRYPTED_BUFFER_LEN 2000
#define DECRYPTED_BUFFER_LEN 2000
#define STRING_BUFFER_LEN 2000

#define MAX_PACKET_SIZE 99999


vfsContext_t ctx;//FIXME: don't use a global 
vfsContext_t *c = &ctx;
AccessTokenState_t accessTokenState;
DriverState_t downloadDrive;

static int getattr_callback(const char *path, struct stat *stbuf) {
	
	printf("getattr_callback: %s\n", path);

	memset( stbuf, 0, sizeof( struct stat ) );

	vfsPathHTTPParserState_t parserState;
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

	vfsPathHTTPParserState_t parserState;
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
	
	printf("read_callback: %s offset: %lu\n", path, offset);

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
