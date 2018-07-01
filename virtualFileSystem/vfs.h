#ifndef VFS_H
#define VFS_H

#include "./hiredis/hiredis.h"


typedef struct {
        //@parentId: if parsing a path that points to a file "/something/here.txt",
        //then the parentId is the id of the folder the file is in. However if 
        //parsing a path that points to folder "/something/here/ or /etc",
        //then the parentId is the id of the parent folder
        long parentId;
        char isDir;//FIXME: REPLACE WITH ENUMS (objectType)
        long id;//id of the file/folder the path points to
} vfsObject_t;

typedef struct {
        char isExistingObject;
        char nameOffset;//offset points to any characters after the last '/' in the file path
        int nameLength;
        //@isFilePath If this is true it DOESN'T mean it's an existing file,
        //only that it's a file path (like "/something.txt as opposed to a directory path like /something/
        char isFilePath;
        char isValidPath;//FIXME: this is never used anwhere, what the fuck?
        int error;
	vfsObject_t fileObj;
} vfsPathParserState_t;

typedef struct {
        vfsObject_t cwd;//id of the file/folder the path points to
        redisContext *dbContext;
} vfsContext_t;

int vfsContext_init( vfsContext_t *ctx );

int vfsContext_free( vfsContext_t *ctx );

void init_vfsPathParserState( vfsPathParserState_t *parserState );

int vfs_parsePath( vfsContext_t *ctx, vfsPathParserState_t *parserState,
		const char *fullPath, int fullPathLength );

int vfs_ls( vfsContext_t *ctx, const vfsObject_t *fileObj, char *outputBuf, 
		int maxBuffSize, int *numRetVals );

void vfs_getDirPath( vfsContext_t *ctx, vfsObject_t *fileObj,
                char *outputBuffer, int outputBufferLength );

void vfs_getCWDPath( vfsContext_t *ctx, char *outputBuffer, 
		int outputBufferLength );

long vfs_getFileSize( vfsContext_t *ctx, const vfsObject_t *fileObj );

char *vfs_listUnixStyle( vfsContext_t *ctx,  const vfsObject_t *fileObj );

int vfs_mkdir( vfsContext_t *ctx, const vfsObject_t *containingFolder, 
        const char *name);

long vfs_createFile( vfsContext_t *ctx, const vfsObject_t *parent, const char *name, long size,
                const char *googleId, const char *webUrl, const char *apiUrl );

int vfs_getFileWebUrl( vfsContext_t *ctx, const vfsObject_t *file, char *outputNameBuffer,
                int outputNameBufferLength);

int vfs_mv( vfsContext_t *ctx, const char *oldPath, const char *newPath);

int vfs_deleteObjectWithPath( vfsContext_t *ctx, const char *path );

int vfs_getDirParent( vfsContext_t *ctx, const vfsObject_t *dir, 
        vfsObject_t *parent);

void vfs_cwd( vfsContext_t *ctx, const vfsObject_t *dir );

void buildDatabaseIfRequired( vfsContext_t *ctx );

#endif /* VFS_H */
