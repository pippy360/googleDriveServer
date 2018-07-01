#include "vfs.h"
#include "vfs_private.h"
#include "vfsPathParser.h"

vfsObject_t getRootCWD() {
	vfsObject_t root;
	root.isDir = 1;
	root.parentId = ROOT_PARENT_ID;
	root.id = ROOT_FOLDER_ID;
	return root;
}

int vfsContext_init( vfsContext_t *ctx ) {
	ctx->cwd = getRootCWD();
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
		fullPathLength, ctx->cwd.id );
}

int vfs_ls( vfsContext_t *ctx, const vfsObject_t *fileObj, char *outputBuf, 
		int maxBuffSize, int *numRetVals ) {

	return __vfs_listDirToBuffer( ctx->dbContext, fileObj->id, outputBuf, 
			maxBuffSize, numRetVals );
}

void vfs_getDirPath( vfsContext_t *ctx, vfsObject_t *fileObj, 
		char *outputBuffer, int outputBufferLength ) {
	vfs_getDirPathFromId( ctx->dbContext, fileObj->id,
                outputBuffer, outputBufferLength );
}

void vfs_getCWDPath( vfsContext_t *ctx, char *outputBuffer, 
		int outputBufferLength ) {
	vfs_getDirPathFromId( ctx->dbContext, ctx->cwd.id,
			outputBuffer, outputBufferLength );
}

long vfs_getFileSize( vfsContext_t *ctx, const vfsObject_t *fileObj ) {
	return vfs_getFileSizeById( ctx->dbContext, fileObj->id );
}

char *vfs_listUnixStyle( vfsContext_t *ctx, const vfsObject_t *fileObj ) {
	return __vfs_listUnixStyle( ctx->dbContext, fileObj->id );
}

int vfs_mkdir( vfsContext_t *ctx, const vfsObject_t *containingFolder, 
	const char *name) {
	return __vfs_mkdir(ctx->dbContext, containingFolder->id, name );
}

long vfs_createFile( vfsContext_t *ctx, const vfsObject_t *parent, const char *name, long size,
		const char *googleId, const char *webUrl, const char *apiUrl ) {
	return __vfs_createFile( ctx->dbContext, parent->id, name, size,
		googleId, webUrl, apiUrl );
}

int vfs_getFileWebUrl( vfsContext_t *ctx, const vfsObject_t *file, char *outputNameBuffer,
		int outputNameBufferLength ) {
	return __vfs_getFileWebUrl( ctx->dbContext, file->id, outputNameBuffer,
		outputNameBufferLength );
}

int vfs_mv( vfsContext_t *ctx, const char *oldPath, const char *newPath ) {
	return __vfs_mv( ctx->dbContext, ctx->cwd.id, oldPath, newPath );
}

int vfs_deleteObjectWithPath( vfsContext_t *ctx, const char *path ) {
	return __vfs_deleteObjectWithPath( ctx->dbContext, path, 
		ctx->cwd.id );
}

//will return -1 if failure, for example if getDirParent is called
//on root
int vfs_getDirParent( vfsContext_t *ctx, const vfsObject_t *dir, 
		vfsObject_t *parent) {

	if ( dir->id == ROOT_FOLDER_ID )
		return 0;

	parent->id = dir->id;
	parent->parentId = __vfs_getDirParent( ctx->dbContext, dir->parentId );
	parent->isDir = 1;

	return 0;
}

void vfs_cwd( vfsContext_t *ctx, const vfsObject_t *dir ) {
	ctx->cwd = *dir;
}

void buildDatabaseIfRequired(  vfsContext_t *ctx ) {
        __buildDatabaseIfRequired( ctx->dbContext );
}
