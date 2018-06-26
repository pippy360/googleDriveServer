#include "vfs.h"

int vfsContext_init( vfsContext_t *ctx ) {
	ctx->cwd = ROOT_FOLDER_ID;
	ctx->dbContext = vfs_connect();
	return 0;
}

int vfsContext_free( vfsContext_t *ctx ) {
	//todo:
	//fixme:
	return 0;
}


int vfs_parsePath( vfsContext_t *ctx, vfsPathParserState_t *parserState,
		const char *fullPath, int fullPathLength) {

	return __vfs_parsePath( ctx->dbContext, parserState, fullPath, 
		fullPathLength, ctx->cwd );
}

int vfs_ls( vfsContext_t *ctx, vfsPathParserState_t *parserState, char *outputBuf, 
		int maxBuffSize, int *numRetVals ) {

	return __vfs_listDirToBuffer( ctx->dbContext, ctx->cwd, outputBuf, maxBuffSize, 
		numRetVals );
}



