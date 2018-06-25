#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../virtualFileSystem/hiredis/hiredis.h"
#include "../virtualFileSystem/vfs.h"
#include "../virtualFileSystem/vfsPathParser.h"


static const char *filepath = "/file";
static const char *filename = "file";
static const char *filecontent = "I'm the content of the only file available there\n";
redisContext *c;

static int getattr_callback(const char *path, struct stat *stbuf) {
	
	memset( stbuf, 0, sizeof( struct stat ) );

	vfsPathParserState_t parserState;
	init_vfsPathParserState( &parserState );
 	vfs_parsePath( c, &parserState, path, strlen(path), 0/*cwd*/ );

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

	char fuseLsbuf[20000];
	int numRetVals;

	//FIXME: get cwd
	long dirId = vfs_getIdByPath(path, 0);
	vfs_fuseLsDir(c, dirId, fuseLsbuf, 999, &numRetVals);
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
        unsigned int j;
        redisReply *reply;
        const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
        int port = (argc > 2) ? atoi(argv[2]) : 6379;

        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        c = redisConnectWithTimeout(hostname, port, timeout);
        if (c == NULL || c->err) {
                if (c) {
                        printf("Connection error: %s\n", c->errstr);
                        redisFree(c);
                } else {
                        printf("Connection error: can't allocate redis context\n");
                }
                exit(1);
        }

  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
