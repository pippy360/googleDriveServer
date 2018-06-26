#ifndef VFSPRIVATE_H
#define VFSPRIVATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "./hiredis/hiredis.h"

#define ROOT_FOLDER_ID 0
#define ROOT_PARENT_ID -1

redisContext *vfs_connect();

int __vfs_listDirToBuffer( redisContext *context, long dirId, char *fuseLsbuf, 
		int maxBuffSize, int *numRetVals );

long vfs_findFileNameInDir(redisContext *context, long dirId, 
		const char *fileName, int fileNameLength);

long vfs_findDirNameInDir(redisContext *context, long dirId,
                const char *dirName, int dirNameLength);

long vfs_getDirParent(redisContext *context, long cwdId);

void vfs_addFileToFileList(redisContext *context, long folderId, long fileId);

void vfs_addDirToFolderList(redisContext *context, long folderId, long fileId);

void vfs_removeIdFromFileList(redisContext *context, long dirId, long removeId);

void vfs_removeIdFromFolderList(redisContext *context, long dirId, long removeId);

void vfs_setFileName(redisContext *context, long id, char *nameBuffer,
                int nameBufferLength);

void vfs_setDirParent(redisContext *context, long dirId, long newParent);

#endif /* VFSPRIVATE_H */
