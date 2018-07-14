#ifndef VFSPATHPARSER_H
#define VFSPATHPARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vfs.h"
#include "vfs_private.h"
#include "./hiredis/hiredis.h"

int __vfs_parsePath( redisContext *context, vfsPathHTTPParserState_t *parserState,
		const char *fullPath, int fullPathLength, long clientCwd );

int __vfs_mv(redisContext *context, long cwd, const char *oldPath, 
	const char *newPath);

int __vfs_deleteObjectWithPath(redisContext *context, const char *path, long cwd);

#endif /* VFSPATHPARSER_H */
