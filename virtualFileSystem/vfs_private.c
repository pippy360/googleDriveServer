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
#define MAX_FILENAME_SIZE 10000

//NO STRING PARSING SHOULD BE DONE IN THIS FILE


//FIXME: mix of i and j for iterators isn't needed and confusing!!!

void vfs_connect(redisContext **c) {
	const char *hostname = "127.0.0.1";
	int port = 6379;

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	*c = redisConnectWithTimeout(hostname, port, timeout);
	if (*c == NULL || (*c)->err) {
		if (*c) {
			printf("Connection error: %s\n", (*c)->errstr);
			redisFree(*c);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		exit(1);
	}
}

long getNewId(redisContext *context) {
	redisReply *reply;
	reply = redisCommand(context, "GET current_id_count");

	//fixme:
	if (reply->str == NULL) {
		return -1;
	}

	long result = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	reply = redisCommand(context, "INCR current_id_count");
	freeReplyObject(reply);
	return result;
}

void __mkdir(redisContext *context, long parentId, long id, char *name) {
	redisReply *reply;
	reply = redisCommand(context,
			"HMSET FOLDER_%lu_info  parent %lu name \"%s\" createdData \"%s\"",
			id, parentId, name, "march 7th");
	freeReplyObject(reply);
}

//returns dir id
long vfs_mkdir(redisContext *context, long parentId, char *name) {
	long id = getNewId(context);
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", parentId, id);
	freeReplyObject(reply);
	__mkdir(context, parentId, id, name);
	return id;
}

void __createFile(redisContext *context, long id, char *name, long size,
		char *googleId, char *webUrl, char *apiUrl) {
	redisReply *reply;
	reply =
			redisCommand(context,
					"HMSET FILE_%lu_info name \"%s\" size \"%lu\" createdData \"%s\" id \"%s\" webUrl \"%s\" apiUrl \"%s\" ",
					id, name, size, "march 7th", googleId, webUrl, apiUrl);
	freeReplyObject(reply);
}

void __addFileToFileList(redisContext *context, long folderId, long fileId) {
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_files %lu", folderId,
			fileId);
	freeReplyObject(reply);
}

void __addDirToFolderList(redisContext *context, long folderId, long fileId) {
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", folderId,
			fileId);
	freeReplyObject(reply);
}

void __removeIdFromFileList(redisContext *context, long dirId, long removeId) {
	redisReply *reply;
	reply = redisCommand(context, "LREM FOLDER_%ld_files 0 %ld", dirId,
			removeId);
	freeReplyObject(reply);
}

void __removeIdFromFolderList(redisContext *context, long dirId, long removeId) {
	redisReply *reply;
	reply = redisCommand(context, "LREM FOLDER_%ld_folders 0 %ld", dirId,
			removeId);
	freeReplyObject(reply);
}

long vfs_createFile(redisContext *context, long parentId, char *name, long size,
		char *googleId, char *webUrl, char *apiUrl) {
	//add it to the file list of the dir
	long id = getNewId(context);
	__addFileToFileList(context, parentId, id);
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

void vfs_setFileName(redisContext *context, long id, char *nameBuffer,
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
int vfs_getFileWebUrl(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
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

long vfs_getFileSizeFromId(redisContext *context, long id) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info size", id);
	if (!reply->str) {
		goto fetch_value_failed;
	}
	printf("we asked for the size of id: %lu and got: %s\n", id, reply->str);
	long size = strtol(reply->str + 1, NULL, 10);//FIXME: the +1 is here because we have the "'s, get rid of that
	return size;

fetch_value_failed:
	freeReplyObject(reply);
	return -1;
}

//^see get file name
int vfs_getFolderName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FOLDER_%lu_info name", id);
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

//this only works with folders for the moment
long vfs_getDirParent(redisContext *context, long cwdId) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FOLDER_%lu_info parent", cwdId);
	//printf("the command we ran HGET FOLDER_%lu_info parent\n", cwdId);
	long newId = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	//printf("the result %ld\n", newId);
	return newId;
}

void vfs_setDirParent(redisContext *context, long dirId, long newParent) {
	redisReply *reply;
	reply = redisCommand(context, "HSET FOLDER_%lu_info parent %ld", dirId,
			newParent);
	freeReplyObject(reply);
}

void vfs_ls(redisContext *context, long dirId) {
	int i = 0;
	long id;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}

			id = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFolderName( context, id, name, MAX_FILENAME_SIZE ) ) {
				printf("ls: %s\n", name);
			} else {
				printf("ERROR: broken folder\n");
			}
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

			long id = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFileName( context, id, name, MAX_FILENAME_SIZE ) ) {
				printf( "ls: %s\n", name );
			} else {
				printf( "ERROR: broken file\n" );
			}
		}
	}
	freeReplyObject(reply);
}

//return's id if successful, -1 otherwise
long vfs_findFileNameInDir(redisContext *context, long dirId, char *fileName,
		int fileNameLength) {
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_files 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			if (!reply->element[i]->str) {
				printf( "ERROR: broken element\n" );
				continue;
			}

			tempId = strtol(reply->element[i]->str, NULL, 10);
			if ( vfs_getFileName( context, tempId, nameBuffer, MAX_FILENAME_SIZE ) ) {
				printf( "ERROR: broken file\n" );
				continue;
			}

			if (strncmp(fileName, nameBuffer, fileNameLength) == 0) {
				matchId = tempId;
			}
		}
	}
	freeReplyObject(reply);
	return matchId;
}

//return's id if successful, -1 otherwise
//works for '.' and '..'
long vfs_findDirNameInDir(redisContext *context, long dirId, char *dirName,
		int dirNameLength) {
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];

	if (dirNameLength == 1 && strncmp(".", dirName, 1) == 0) {
		return dirId;
	} else if (dirNameLength == 2 && strncmp("..", dirName, 2) == 0) {
		return vfs_getDirParent(context, dirId);
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

int vfs_fuseLsDir(redisContext *context, long dirId, char *fuseLsbuf, int maxBuffSize, 
		int *numRetVals) {
	int i = 0;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	char *ptr = fuseLsbuf;
	int rc;
	numRetVals = 0;
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
			rc = sprintf(ptr, name);
			ptr += rc + 1;
			numRetVals += 1;
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
			rc = sprintf(ptr, name);
			ptr += rc + 1;
			numRetVals += 1;
		}
	}
	freeReplyObject(reply);

	return 0;//fixme, only one return code? then this isn't needed
	
}

//FIXME: SO MUCH REPEATED CODE HERE, CLEAN THIS UP
//-1 if not found, id otherwise

char *vfs_listUnixStyle(redisContext *context, long dirId) {
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
			long fileSize = vfs_getFileSizeFromId(context, fileId);
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
void vfs_getDirPathFromId(redisContext *context, long inputId,
		char *outputBuffer, int outputBufferLength) {
	long currentId = inputId;
	redisReply *parentIdReply, *nameReply;
	char buffer[outputBufferLength];
	char buffer2[outputBufferLength];
	outputBuffer[0] = '\0';

	while (currentId != 0) {
		//get the name of the current id
		vfs_getFolderName(context, currentId, buffer2, outputBufferLength);

		//FIXME: check if it'll fit !
		//if(strlen(outputBuffer) > outputBufferLength){
		//}
		sprintf(buffer, "/%s%s", buffer2, outputBuffer);
		strcpy(outputBuffer, buffer);

		//get it's parent's id
		parentIdReply = redisCommand(context, "HGET FOLDER_%lu_info parent",
				currentId);
		currentId = strtol(parentIdReply->str + 1, NULL, 10);
		freeReplyObject(parentIdReply);
	}
	strcat(outputBuffer, "/");
}

//id 0 == root
void vfs_buildDatabase(redisContext *context) {
	//wipe it and set the first id to 0 and crate a root folder
	redisReply *reply;
	reply = redisCommand(context, "FLUSHALL");
	freeReplyObject(reply);
	__mkdir(context, 0, 0, "root");
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

//FIXME:
void buildDatabaseIfRequired(redisContext *context) {
	if (!isVirtualFileSystemCreated(context)) {
		vfs_buildDatabase(context);
	}
}

#include "vfsPathParser.h"

//int main(int argc, char const *argv[]) {
int something(int argc, char const *argv[]) {
	unsigned int j;
	redisContext *c;
	redisReply *reply;
	const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
	int port = (argc > 2) ? atoi(argv[2]) : 6379;

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout(hostname, port, timeout);
	if (c == NULL || c->err) {
		if (c) {
			printf("Connection error: %s\n", c->errstr);
			redisFree(c);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		exit(1);
	}

	printf("building the database\n");
	;
	vfs_buildDatabase(c);
	printf("is the database set? %d \n", isVirtualFileSystemCreated(c));
	long newDirId = vfs_mkdir(c, 0, "new folder");
	long newFileId = vfs_createFile(c, newDirId, "a_new_file.webm", 1000,
			"something", "www.something.com", "");
	newDirId = vfs_mkdir(c, newDirId, "foldhere");
	newFileId = vfs_createFile(c, newDirId, "far_down_file.webm", 1000,
			"something", "www.something.com", "");

	vfs_ls(c, 0);
	vfs_ls(c, 1);
	vfs_ls(c, 3);
	signed long tempId;
	printf("idFromPath: %s, id: %ld\n", "/", tempId);
	char buffer[10000];
	printf("FolderPathFromId: %lu , path: \"%s\"\n", (long) 0, buffer);
	//pretty print the files and folders
	//list the parts !!
	printf("calling pwd now\n");
	printf("%s", vfs_listUnixStyle(c, 0));
	vfs_ls(c, newDirId);

	printf("new stuff....\n");

	long tempId1, tempId2, tempId3;
	tempId1 = vfs_findDirNameInDir(c, 0, "new folder", strlen("new folder"));
	printf("The id %ld\n", tempId1);

	printf("lets find foldhere\n");
	tempId3 = vfs_findFileNameInDir(c, 0, "foldhere", strlen("foldhere"));
	printf("The id %ld\n", tempId3);
	tempId1 = vfs_findDirNameInDir(c, 0, "new folder", strlen("new folder"));
	printf("The id %ld\n", tempId1);
	printf("let's print the /new folder/\n");
	vfs_ls(c, tempId1);
	tempId3 = vfs_findDirNameInDir(c, tempId1, "foldhere", strlen("foldhere"));
	printf("The id %ld\n", tempId3);

	tempId1 = vfs_findFileNameInDir(c, 0, "far_down_file.webm",
			strlen("far_down_file.webm"));
	printf("The id %ld\n", tempId1);

	printf("let's print the /new folder/foldhere/\n");
	vfs_ls(c, tempId3);

	tempId1 = vfs_findFileNameInDir(c, tempId3, "far_down_file.webm",
			strlen("far_down_file.webm"));
	printf("The id %ld\n", tempId1);

	vfsPathParserState_t parserState, parserState2;
	init_vfsPathParserState(&parserState);
	vfs_findObjectInDir(c, &parserState, tempId3, "far_down_file.webm",
			strlen("far_down_file.webm"));
	printf("is it a file??? %d\n", parserState.isFilePath);

	char *str1 = "sadf/";
	vfs_serperatePathAndName(&parserState, str1, strlen(str1));
	printf("the last bit of %s - %d %s\n", str1, parserState.nameLength,
			str1 + parserState.nameOffset);

	init_vfsPathParserState(&parserState);
	char *tempPath = "/new folder/foldhere/sdfsdf/";
	printf("lets get the dir id using a path\n");
	printf("the path %s\n", tempPath);
	printf("tempId3 before %ld\n", tempId3);
	tempId3 = vfs_getDirIdFromPath(c, 0, tempPath, strlen(tempPath));
	printf("tempId3 after %ld\n", tempId3);
	vfs_findObjectInDir(c, &parserState, tempId3, "far_down_file.webm",
			strlen("far_down_file.webm"));
	printf("is it a file??? %d\n", parserState.isFilePath);

	char *str2 = "/new folder/foldhere";
	init_vfsPathParserState(&parserState);
	int result1 = vfs_parsePath(c, &parserState, str2, strlen(str2), 0);
	if (result1 == 0) {
		printf("id                  %ld\n", parserState.id);
		printf("parent Id           %ld\n", parserState.parentId);
		printf("isFile              %d\n", parserState.isFilePath);
		printf("isDir               %d\n", parserState.isDir);
		printf("nameLength          %d\n", parserState.nameLength);
		printf("name:               %s\n", str2 + parserState.nameOffset);
	} else {
		printf("Bad path!\n");
	}

	vfs_mv(c, 0, "./././new folder/../new folder/foldhere", "./././././");
	vfs_ls(c, 0);
	str2 = "/foldhere";
	init_vfsPathParserState(&parserState);
	result1 = vfs_parsePath(c, &parserState, str2, strlen(str2), 0);
	vfs_debug_printParserState(&parserState);

//	vfs_mv(c, 0, "./new folder/foldhere/", "./")
//	vfs_ls(c, 0);

	str2 = "/foldhere";
	init_vfsPathParserState(&parserState);
	result1 = vfs_parsePath(c, &parserState, str2, strlen(str2), 0);
	vfs_debug_printParserState(&parserState);
	printf("testing '..'");
	long tempId4;
	tempId4 = parserState.id;
	init_vfsPathParserState(&parserState);
	result1 = vfs_parsePath(c, &parserState, "..", strlen(".."), tempId4);
	vfs_debug_printParserState(&parserState);

	vfs_ls(c, tempId4);

	char *str8, *str9;
	str8 = "far_down_file.webm";
	str9 = "something.jpg";

	/*
	 printf("tempId4 %ld\n", tempId4);
	 printf("starting the mv\n");
	 printf("################1\n");
	 result1 = vfs_parsePath(c, &parserState, str8, strlen(str8), tempId4);
	 printf("################2\n");
	 vfs_debug_printParserState(&parserState);


	 init_vfsPathParserState(&parserState2);
	 result1 = vfs_parsePath(c, &parserState2, str9, strlen(str9), tempId4);
	 printf("the mv is done\n");
	 vfs_debug_printParserState(&parserState2);

	 vfsPathParserState_t o, n;
	 o = parserState;
	 n = parserState2;
	 if (o.isFile && !n.isExistingObject && 1)
	 printf("the conditions are met\n");
	 //ok we have the details
	 printf("before\n");
	 vfs_ls(c, tempId4);
	 __removeIdFromFileList(c, o.parentId, o.id);
	 printf("after\n");
	 vfs_ls(c, tempId4);

	 __addFileToFileList(c, n.parentId, o.id);
	 printf("setting file name --%.*s--\n", n.nameLength,
	 str9 + n.nameOffset);


	 //HSET FILE_4_info name "something.jpg"
	 redisReply *reply2;
	 reply2 = redisCommand(c, "HSET FILE_4_info name \"something.jpg\"", str9);
	 */
	vfs_mv(c, tempId4, str8, str9);
	vfs_ls(c, tempId4);

	return 0;
}

