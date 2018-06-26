#ifndef VFSPATHPARSER_H
#define VFSPATHPARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vfs_private.h"
#include "./hiredis/hiredis.h"


typedef struct {
	long cwd;//id of the file/folder the path points to
	redisContext *dbContext;
} vfsContext_t;


typedef struct {
	char isExistingObject;
	char nameOffset;//offset points to any characters after the last '/' in the file path
	int nameLength;
	//@isFilePath If this is true it DOESN'T mean it's an existing file,
	//only that it's a file path (like "/something.txt as opposed to a directory path like /something/
	char isFilePath;
	char isDir;//FIXME: REPLACE WITH ENUMS (objectType)
	char isValid;
	int error;
	//parentId: if parsing a path that points to a file "/something/here.txt",
	//then the parentId is the id of the folder the file is in
	//parentId: if parsing a path that points to folder "/something/here/ or /etc",
	//then the parentId is the id of the parent folder
	long parentId;
	long id;//id of the file/folder the path points to
} vfsPathParserState_t;

int __vfs_parsePath(redisContext *context, vfsPathParserState_t *parserState,
		const char *fullPath, int fullPathLength, long clientCwd);

#endif /* VFSPATHPARSER_H */
