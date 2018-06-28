#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../virtualFileSystem/hiredis/hiredis.h"
#include "../virtualFileSystem/vfs.h"


static const char *filepath = "/file";
static const char *filecontent = "I'm the content of the only file available there\n";
vfsContext_t ctx;
vfsContext_t *c = &ctx;

static int getattr_callback(const char *path, struct stat *stbuf) {
	
	printf("2the path they asked for: %s\n", path);

	memset( stbuf, 0, sizeof( struct stat ) );

	vfsPathParserState_t parserState;
	init_vfsPathParserState( &parserState );
 	vfs_parsePath( c, &parserState, path, strlen(path) );

	if ( parserState.isDir ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	} else {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(filecontent);
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

  if (strcmp(path, filepath) == 0) {
    size_t len = strlen(filecontent);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
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
  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
