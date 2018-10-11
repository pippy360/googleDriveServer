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
		const char *fullPath ) {

	return __vfs_parsePath( ctx->dbContext, parserState, fullPath, 
		strlen( fullPath ), ctx->cwd.id );
}

int vfs_parsePathOfLength( vfsContext_t *ctx, vfsPathParserState_t *parserState,
		const char *fullPath, int fullPathLength) {

	return __vfs_parsePath( ctx->dbContext, parserState, fullPath, 
		fullPathLength, ctx->cwd.id );
}

int vfs_ls( vfsContext_t *ctx, const vfsObject_t *file, char *outputBuf, 
		int maxBuffSize, int *numRetVals ) {

	return __vfs_listDirToBuffer( ctx->dbContext, file->id, outputBuf, 
			maxBuffSize, numRetVals );
}

void vfs_getDirPath( vfsContext_t *ctx, vfsObject_t *file, 
		char *outputBuffer, int outputBufferLength ) {
	vfs_getDirPathFromId( ctx->dbContext, file->id,
                outputBuffer, outputBufferLength );
}

void vfs_getCWDPath( vfsContext_t *ctx, char *outputBuffer, 
		int outputBufferLength ) {
	vfs_getDirPathFromId( ctx->dbContext, ctx->cwd.id,
			outputBuffer, outputBufferLength );
}

long vfs_getFileSize( vfsContext_t *ctx, const vfsObject_t *file ) {
	long ret =  vfs_getFileSizeById( ctx->dbContext, file->id );
	printf( "vfs_getFileSize called. File id: %lu with size %lu \n", file->id, ret );
	return ret;
}

char *vfs_listUnixStyle( vfsContext_t *ctx, const vfsObject_t *file ) {
	return __vfs_listUnixStyle( ctx->dbContext, file->id );
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

long vfs_createNewEmptyFile( vfsContext_t *ctx, const char *path ) {
	
	printf( "vfs asked to create a new file with path %s\n", path );

	vfsPathParserState_t parserState;
	int retVal = 0;
	if( ( retVal = vfs_parsePath( ctx, &parserState, path ) ) ) {
		printf("Failed to parse path retval: %d \n", retVal);
		return retVal;
	}

	if( parserState.fileObj.parentId ) {//parent folder doesn't exist
		printf("The parent the path points to doesn't exist\n");
		return -1;
	}

	if( parserState.isExistingObject ) {//file already exists
		printf("File already exists\n");
		return -1;
	}

	long size = 0;
	char *webUrl = "";
	char *googleId = "";
	char *apiUrl = "";
	printf("adding file name: %s to parent of id %ld ", path + parserState.nameOffset, parserState.parentObj.id );
	return vfs_createFile( ctx, &parserState.parentObj, path + parserState.nameOffset,
		       size, googleId, webUrl, apiUrl );
}

int vfs_getFileWebUrl( vfsContext_t *ctx, const vfsObject_t *file, char *outputNameBuffer,
		int outputNameBufferLength ) {
	printf( "vfs_getFileWebUrl called. File id: %lu\n", file->id );
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

//will return -1 if failure, for example if getDirParent is called on root
int vfs_getParent( vfsContext_t *ctx, const vfsObject_t *obj, vfsObject_t *parent) {
	printf( "vfs_getDirParent called. Dir id: %ld\n", obj->id );
	if ( obj->id == ROOT_FOLDER_ID )
		return 0;

	parent->id = obj->parentId;
	parent->isDir = 1;
	parent->parentId = __vfs_getDirParent( ctx->dbContext, obj->parentId );
	if ( parent->parentId == -1 ) {
		printf( "vfs_getParent failed to get parent Id for %ld\n", obj->parentId );
		return -1;
	}
	return 0;
}

void vfs_set_cwd( vfsContext_t *ctx, const vfsObject_t *dir ) {
	ctx->cwd = *dir;
}

void buildDatabaseIfRequired(  vfsContext_t *ctx ) {
        __buildDatabaseIfRequired( ctx->dbContext );
}
