#include "./hiredis/hiredis.h"
#include "vfs_private.h"
#include "vfsPathParser.h"

int vfsContext_init( vfsContext_t *ctx );

int vfsContext_free( vfsContext_t *ctx );

int vfs_parsePath( vfsContext_t *ctx, vfsPathParserState_t *parserState,
		char *fullPath, int fullPathLength);

int vfs_ls( vfsContext_t *ctx, vfsPathParserState_t *parserState, char *outputBuf, int maxBuffSize, 
		int *numRetVals );



