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

char *vfs_listUnixStyle(redisContext *context, long dirId);

long vfs_createFile(redisContext *context, long parentId, char *name, long size,
		char *id, char *webUrl, char *apiUrl);

void vfs_getFileName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength);

void vfs_getFileWebUrl(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength);
