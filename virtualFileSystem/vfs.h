char *vfs_listUnixStyle(redisContext *context, long dirId);

long vfs_createFile(redisContext *context, long parentId, char *name, long size,
		char *id, char *webUrl, char *apiUrl);

void vfs_getFileName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength);

void vfs_getFileWebUrl(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength);
