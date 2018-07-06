#include "vfs_private.h"

#define MAX_FILENAME_SIZE 10000

//NO STRING PARSING SHOULD BE DONE IN THIS FILE


//FIXME: mix of i and j for iterators isn't needed and confusing!!!

redisContext *vfs_connect() {
	const char *hostname = "127.0.0.1";
	int port = 6379;

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	redisContext *c = redisConnectWithTimeout(hostname, port, timeout);
	if (c == NULL || (c)->err) {
		if (c) {
			printf("Connection error: %s\n", (c)->errstr);
			redisFree(c);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		exit(1);
	}
	return c;
}

long getNewId( redisContext *context ) {
	redisReply *reply;
	reply = redisCommand( context, "GET current_id_count" );

	//fixme:
	if ( reply->str == NULL ) {
		return -1;
	}

	long result = strtol( reply->str, NULL, 10 );
	freeReplyObject( reply );
	reply = redisCommand( context, "INCR current_id_count" );
	freeReplyObject( reply );
	return result;
}

void __private_mkdir(redisContext *context, long parentId, long id, const char *name) {
	redisReply *reply;
	reply = redisCommand(context,
			"HMSET FOLDER_%lu_info  parent %lu name \"%s\" createdData \"%s\"",
			id, parentId, name, "march 7th");
	freeReplyObject(reply);
}

//returns dir id
long __vfs_mkdir(redisContext *context, long parentId, const char *name) {
	long id = getNewId(context);
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", parentId, id);
	freeReplyObject(reply);
	__private_mkdir(context, parentId, id, name);
	return id;
}

void __createFile(redisContext *context, long id, const char *name, long size,
		const char *googleId, const char *webUrl, const char *apiUrl) {
	redisReply *reply;
	reply =
			redisCommand(context,
					"HMSET FILE_%lu_info name \"%s\" size \"%lu\" createdData \"%s\" id \"%s\" webUrl \"%s\" apiUrl \"%s\" ",
					id, name, size, "march 7th", googleId, webUrl, apiUrl);
	freeReplyObject(reply);
}

void vfs_addFileToFileList(redisContext *context, long folderId, long fileId) {
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_files %lu", folderId,
			fileId);
	freeReplyObject(reply);
}

void vfs_addDirToFolderList(redisContext *context, long folderId, long fileId) {
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", folderId,
			fileId);
	freeReplyObject(reply);
}

void vfs_removeIdFromFileList(redisContext *context, long dirId, long removeId) {
	redisReply *reply;
	reply = redisCommand(context, "LREM FOLDER_%ld_files 0 %ld", dirId,
			removeId);
	freeReplyObject(reply);
}

void vfs_removeIdFromFolderList(redisContext *context, long dirId, long removeId) {
	redisReply *reply;
	reply = redisCommand(context, "LREM FOLDER_%ld_folders 0 %ld", dirId,
			removeId);
	freeReplyObject(reply);
}

long __vfs_createFile(redisContext *context, long parentId, const char *name, long size,
		const char *googleId, const char *webUrl, const char *apiUrl) {
	//add it to the file list of the dir
	long id = getNewId(context);
	vfs_addFileToFileList(context, parentId, id);
	__createFile(context, id, name, size, googleId, webUrl, apiUrl);
	return id;
}

void __deleteFile(redisContext *context, long fileId) {

}

void __deleteDir(redisContext *context, long dirId) {

}

//FIXME: return error codes
void vfs_delete(redisContext *context, char *path) {

}

void __mv(redisContext *context, long fileId, long oldParentId,
		long newParentId, char *newFileName) {
	if (newFileName == NULL) {

	}
}

void vfs_setFileName(redisContext *context, long id, const char *nameBuffer,
		int nameBufferLength) {
	redisReply *reply;
	//FIXME: using %.*s doesn't work when used with redisCommand....weird...
	//so we have to do this hack below
	char *tempPtr = malloc(nameBufferLength + 1);
	memcpy(tempPtr, nameBuffer, nameBufferLength);
	tempPtr[nameBufferLength] = '\0';

	reply = redisCommand(context, "HSET FILE_%lu_info name \"%s\"", id,
			tempPtr);
	freeReplyObject(reply);
	free(tempPtr);
}

//  ####  ###### ##### ##### ###### #####   ####
// #    # #        #     #   #      #    # #
// #      #####    #     #   #####  #    #  ####
// #  ### #        #     #   #      #####       #
// #    # #        #     #   #      #   #  #    #
//  ####  ######   #     #   ###### #    #  ####

//returns the reply for the name resquest
//so use freeReplayObject() to free
int vfs_getFileName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info name", id);
	if (!reply->str) {
		goto fetch_value_failed;
	}
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
	return 0;

fetch_value_failed:
	freeReplyObject(reply);
	return -1;
}

//FIXME: FIXME: temporary fix here, change APIURL TO WEBURL
int __vfs_getFileWebUrl(redisContext *context, long id, 
	char *outputNameBuffer, int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info apiUrl", id);
	if (!reply->str) {
		goto fetch_value_failed;
	}

	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
	return 0;

fetch_value_failed:
	freeReplyObject(reply);
	return -1;
}

long vfs_getFileSizeById( redisContext *context, long id ) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info size", id);
	if (!reply->str) {
		goto fetch_value_failed;
	}
	long size = strtol(reply->str + 1, NULL, 10);//FIXME: the +1 is here because we have the "'s, get rid of that
	return size;

fetch_value_failed:
	freeReplyObject(reply);
	return -1;
}

//^see get file name
int vfs_getFolderName( redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength ) {
	redisReply *reply;
	reply = redisCommand( context, "HGET FOLDER_%lu_info name", id );
	if ( !reply->str ) {
		goto fetch_value_failed;
	}
	sprintf( outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1 );
	freeReplyObject( reply );
	return 0;

fetch_value_failed:
	freeReplyObject( reply );
	return -1;
}

//this only works with folders for the moment
long __vfs_getDirParent( redisContext *context, long dirId ) {
	redisReply *reply;
	reply = redisCommand( context, "HGET FOLDER_%lu_info parent", dirId );
	long newId = strtol( reply->str, NULL, 10 );
	freeReplyObject( reply );
	return newId;
}

void __vfs_setDirParent( redisContext *context, long dirId, long newParent ) {
	redisReply *reply;
	reply = redisCommand( context, "HSET FOLDER_%lu_info parent %ld", dirId,
			newParent );
	freeReplyObject( reply );
}

void __vfs_ls_print( redisContext *context, long dirId ) {
	int i = 0;
	long id;
	redisReply *reply;
	char name[ MAX_FILENAME_SIZE ];
	reply = redisCommand( context, "LRANGE FOLDER_%lu_folders 0 -1", dirId );
	if ( reply->type == REDIS_REPLY_ARRAY ) {
		for ( i = 0; i < reply->elements; i++ ) {
			if ( !reply->element[i]->str ) {
				printf( "ERROR: broken element\n" );
				continue;
			}

			id = strtol( reply->element[i]->str, NULL, 10 );
			if ( vfs_getFolderName( context, id, name, MAX_FILENAME_SIZE ) ) {
				printf( "ls: %s\n", name );
			} else {
				printf( "ERROR: broken folder\n" );
			}
		}
	}
	freeReplyObject( reply );
	reply = redisCommand( context, "LRANGE FOLDER_%lu_files   0 -1", dirId );
	if ( reply->type == REDIS_REPLY_ARRAY ) {
		for ( i = 0; i < reply->elements; i++ ) {
			if ( !reply->element[i]->str ) {
				printf( "ERROR: broken element\n" );
				continue;
			}

			long id = strtol( reply->element[i]->str, NULL, 10 );
			if ( vfs_getFileName( context, id, name, MAX_FILENAME_SIZE ) ) {
				printf( "ls: %s\n", name );
			} else {
				printf( "ERROR: broken file\n" );
			}
		}
	}
	freeReplyObject( reply );
}

//return's id if successful, -1 otherwise
long vfs_findFileNameInDir( redisContext *context, long dirId, 
		const char *fileName, int fileNameLength ) {
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[ MAX_FILENAME_SIZE ];
	reply = redisCommand( context, "LRANGE FOLDER_%lu_files 0 -1", dirId );
	if ( reply->type == REDIS_REPLY_ARRAY ) {
		for ( i = 0; i < reply->elements; i++ ) {
			if ( !reply->element[i]->str ) {
				printf( "ERROR: broken element\n" );
				continue;
			}

			tempId = strtol( reply->element[i]->str, NULL, 10 );
			if ( vfs_getFileName( context, tempId, nameBuffer, MAX_FILENAME_SIZE ) ) {
				printf( "ERROR: broken file\n" );
				continue;
			}

			if ( strncmp( fileName, nameBuffer, fileNameLength ) == 0 ) {
				matchId = tempId;
			}
		}
	}
	freeReplyObject( reply );
	return matchId;
}

//return's id if successful, -1 otherwise
//works for '.' and '..'
long vfs_findDirNameInDir(redisContext *context, long dirId, 
		const char *dirName, int dirNameLength) {
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];

	if (dirNameLength == 1 && strncmp(".", dirName, 1) == 0) {
		return dirId;
	} else if (dirNameLength == 2 && strncmp("..", dirName, 2) == 0) {
		return __vfs_getDirParent(context, dirId);
	}

	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}

			tempId = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFolderName( context, tempId, nameBuffer, MAX_FILENAME_SIZE ) ) {
				printf( "ERROR: broken folder\n" );
				continue;
			}

			if (strncmp(dirName, nameBuffer, dirNameLength) == 0) {
				matchId = tempId;
			}
		}
	}
	freeReplyObject(reply);
	return matchId;
}

int __vfs_listDirToBuffer( redisContext *context, long dirId, char *fuseLsbuf, int maxBuffSize, 
		int *numRetVals ) {
	int i = 0;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	char *ptr = fuseLsbuf;
	int rc;
	*numRetVals = 0;
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}
			long innerDirId = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFolderName(context, innerDirId, name, MAX_FILENAME_SIZE) ) {
				printf( "ERROR: broken folder\n" );
				continue;
			}
			rc = sprintf(ptr, "%s", name);
			ptr += rc + 1;
			*numRetVals += 1;
		}
	}
	freeReplyObject(reply);

	reply = redisCommand(context, "LRANGE FOLDER_%lu_files   0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}
			long fileId = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFileName(context, fileId, name, MAX_FILENAME_SIZE) ) {
				printf( "ERROR: broken file\n" );
				continue;
			}
			rc = sprintf(ptr, "%s", name);
			ptr += rc + 1;
			*numRetVals += 1;
		}
	}
	freeReplyObject(reply);

	return 0;//fixme, only one return code? then this isn't needed
	
}

//FIXME: SO MUCH REPEATED CODE HERE, CLEAN THIS UP
//-1 if not found, id otherwise

char *__vfs_listUnixStyle(redisContext *context, long dirId) {
	char *line = malloc(20000);//fixme:
	line[0] = '\0';

	int i = 0;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}
			long innerDirId = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFolderName(context, innerDirId, name, MAX_FILENAME_SIZE) ) {
				printf( "ERROR: broken folder\n" );
				continue;
			}
			sprintf(line + strlen(line),
					"%s   1 %s %s %10lu Jan  1  1980 %s\r\n", "drwxrwxr-x",
					"linux", "linux", (long) 1, name);
		}
	}
	freeReplyObject(reply);

	reply = redisCommand(context, "LRANGE FOLDER_%lu_files   0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}
			long fileId = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFileName(context, fileId, name, MAX_FILENAME_SIZE) ) {
				printf( "ERROR: broken file\n" );
				continue;
			}
			long fileSize = vfs_getFileSizeById(context, fileId);
			sprintf(line + strlen(line),
					"%s   1 %s %s %10lu Jan  1  1980 %s\r\n", "-rwxrwxr-x",
					"linux", "linux", fileSize, name);
		}
	}
	freeReplyObject(reply);

	return line;
}

//FIXME: MOVE THIS TO THE PARSER
//FIXME: clean up the use of buffers here, buffer2 is kind of a hack
void vfs_getDirPathFromId( redisContext *context, long inputId,
		char *outputBuffer, int outputBufferLength ) {
	long currentId = inputId;
	redisReply *parentIdReply;
	char buffer[outputBufferLength];
	char buffer2[outputBufferLength];
	outputBuffer[0] = '\0';

	while ( currentId != 0 ) {
		//get the name of the current id
		vfs_getFolderName( context, currentId, buffer2, outputBufferLength );

		//FIXME: check if it'll fit !
		//if(strlen(outputBuffer) > outputBufferLength){
		//}
		sprintf( buffer, "/%s%s", buffer2, outputBuffer );
		strcpy( outputBuffer, buffer );

		//get it's parent's id
		parentIdReply = redisCommand( context, 
				"HGET FOLDER_%lu_info parent", currentId );
		currentId = strtol( parentIdReply->str + 1, NULL, 10 );
		freeReplyObject( parentIdReply );
	}
	strcat( outputBuffer, "/" );
}

//id 0 == root
void vfs_buildDatabase(redisContext *context) {
	//wipe it and set the first id to 0 and crate a root folder
	redisReply *reply;
	reply = redisCommand(context, "FLUSHALL");
	freeReplyObject(reply);
	__private_mkdir(context, 0, 0, "root");
	reply = redisCommand(context, "SET current_id_count 1");
	reply = redisCommand(context, "SET isVirtualFileSystemCreated yes");
	freeReplyObject(reply);
}

int isVirtualFileSystemCreated(redisContext *context) {
	redisReply *reply;
	reply = redisCommand(context, "EXISTS isVirtualFileSystemCreated");
	long newId = (int) reply->integer;
	freeReplyObject(reply);
	return newId;
}

void __buildDatabaseIfRequired(redisContext *context) {
	if (!isVirtualFileSystemCreated(context)) {
		vfs_buildDatabase(context);
	}
}
