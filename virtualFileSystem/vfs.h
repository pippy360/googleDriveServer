
char *vfs_listUnixStyle(redisContext *context, long dirId);

long vfs_createFile(redisContext *context, long parentId, char *name, long size, char *id, char *webUrl);
