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
	long result = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	reply = redisCommand(context, "INCR current_id_count");
	freeReplyObject(reply);
	return result;
}

void __mkdir(redisContext *context, long parentId, long id, char *name) {
	redisReply *reply;
	reply =
			redisCommand(context,
					"HMSET FOLDER_%lu_info  parent \"%lu\" name \"%s\" createdData \"%s\"",
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

//id 0 == root
void vfs_buildDatabase(redisContext *context) {
	//wipe it and set the first id to 0 and crate a root folder
	redisReply *reply;
	reply = redisCommand(context, "FLUSHALL");
	freeReplyObject(reply);
	__mkdir(context, 0, 0, "root");
	reply = redisCommand(context, "SET current_id_count 1");
	freeReplyObject(reply);
}

//returns the reply for the name resquest
//so use freeReplayObject() to free
void vfs_getFileName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info name", id);
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
}

//FIXME: FIXME: temporary fix here, change APIURL TO WEBURL
void vfs_getFileWebUrl(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info apiUrl", id);
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
}

long vfs_getFileSizeFromId(redisContext *context, long id) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info size", id);
	if (reply->str == NULL) {
		freeReplyObject(reply);
		return -1;
	}
	long size = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	return size;
}

//^see get file name
void vfs_getFolderName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FOLDER_%lu_info name", id);
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
}

//this only works with folders for the moment
long vfs_getParent(redisContext *context, long cwdId) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FOLDER_%lu_info parent", cwdId);
	long newId = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	return newId;
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

long vfs_createFile(redisContext *context, long parentId, char *name, long size,
		char *googleId, char *webUrl, char *apiUrl) {
	//add it to the file list of the dir
	long id = getNewId(context);
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_files %lu", parentId, id);
	freeReplyObject(reply);
	__createFile(context, id, name, size, googleId, webUrl, apiUrl);
	return id;
}

//return part id
void vfs_createPart(redisContext *context, long fileId, char *partUrl,
		long rangeStart, long rangeEnd) {
	//add it to the list
}

//FIXME: return error codes
void vfs_delete(redisContext *context, char *path){

}

void __deleteFile(redisContext *context, long fileId){

}

void __deleteDir(redisContext *context, long dirId){

}

void __createPart() {
	//creates the info part
}

void vfs_ls(redisContext *context, long dirId) {
	int j = 0;
	long id;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFolderName(context, id, name, MAX_FILENAME_SIZE);
			printf("%s\n", name);
		}
	}
	freeReplyObject(reply);
	reply = redisCommand(context, "LRANGE FOLDER_%lu_files   0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			long id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFileName(context, id, name, MAX_FILENAME_SIZE);
			printf("%s\n", name);
		}
	}
	freeReplyObject(reply);
}

void vfs_rmDir() {

}

void vfs_rmFile() {

}

//FIXME: SO MUCH REPEATED CODE HERE, CLEAN THIS UP
//-1 if not found, id otherwise
long vfs_getFileIdByName(redisContext *context, long parentFolderId,
		char *inputName) {
	int j = 0;
	long id, result = -1;
	char nameBuffer[MAX_FILENAME_SIZE];
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_files 0 -1",
			parentFolderId);

	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFileName(context, id, name, MAX_FILENAME_SIZE);
			if (strcmp(name, inputName) == 0) {
				result = id;
			}
		}
	}
	freeReplyObject(reply);
	return result;
}

//-1 if not found, id otherwise
long vfs_getFolderIdByName(redisContext *context, long parentFolderId,
		char *inputName) {
	int j = 0;
	long id, result = -1;
	char name[MAX_FILENAME_SIZE];
	redisReply *reply;
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1",
			parentFolderId);

	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFolderName(context, id, name, MAX_FILENAME_SIZE);
			if (strcmp(name, inputName) == 0) {
				result = id;
			}
		}
	}
	freeReplyObject(reply);
	return result;
}

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

//path must start with '/'
long vfs_getFileIdFromPath(redisContext *context, char *path) {

	char *ptr = path + 1;
	long resultId = 0;
	char buffer[MAX_FILENAME_SIZE];

	if (path[0] != '/')
		return -1;

	while (*ptr) {
		//get the name and search it
		if(__stripName(ptr, buffer, MAX_FILENAME_SIZE) != 0){
			return -1;
		}
		//if the name isn't found return -1
		if ((resultId = vfs_getFileIdByName(context, resultId, buffer)) == -1) {
			printf("Id not found\n");
			return -1;
		}
		//jump the '/'
		if (*ptr == '/')
			ptr++;
	}
	return resultId;
}

//path must start with '/'
long vfs_getDirIdFromPath(redisContext *context, char *path) {

	char *ptr = path + 1;
	long resultId = 0;
	char buffer[MAX_FILENAME_SIZE];

	if (path[0] != '/')
		return -1;

	while (*ptr) {
		//get the name and search it
		if(__stripName(ptr, buffer, MAX_FILENAME_SIZE) != 0){
			return -1;
		}
		//if the name isn't found return -1
		if ((resultId = vfs_getDirIdByName(context, resultId, buffer)) == -1) {
			printf("Id not found\n");
			return -1;
		}
		//jump the '/'
		if (*ptr == '/')
			ptr++;
	}
	return resultId;
}


//startPtr should start at the start of the name
//returns 0 if success, non-0 otherwise
int __stripName(char *startPtr, char *outputBuffer, int outputBufferMaxLength){
	int i;
	for (i = 0; *startPtr && *startPtr != '/'; i++, startPtr++) {
		if (!isprint(*startPtr)) {
			return -1;
		}
		outputBuffer[i] = *startPtr;
	}
	outputBuffer[i] = '\0';
	return 0;
}

long vfs_getDirIdFromRelPath(redisContext *vfsContext, long cwdId, char *relPath) {

	char newPath[10000];
	char newPath2[10000];
	printf("relPath: %s\n", relPath);
	vfs_getDirPathFromId(vfsContext, cwdId, newPath, 10000);
	sprintf(newPath2, "%s%s/", newPath, relPath);
	printf("newPath: %s\n", newPath2);
	return vfs_getDirIdFromPath(vfsContext, newPath2);
}

long vfs_getFileIdFromRelPath(redisContext *vfsContext, long cwdId, char *relPath) {

	char newPath[10000];
	char newPath2[10000];
	printf("relPath: %s\n", relPath);
	vfs_getDirPathFromId(vfsContext, cwdId, newPath, 10000);
	sprintf(newPath2, "%s%s/", newPath, relPath);
	printf("newPath: %s\n", newPath2);
	return vfs_getFileIdFromPath(vfsContext, newPath2);
}

char *vfs_listUnixStyle(redisContext *context, long dirId) {
	//get the folder
	//get all the id's, go over them and print sprintf the contents

	//get the folder
	//go through all the folders
	char *line = malloc(20000);
	line[0] = '\0';

	int j = 0;
	long id;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFolderName(context, id, name, MAX_FILENAME_SIZE);
			sprintf(line + strlen(line),
					"%s   1 %s %s %10lu Jan  1  1980 %s\r\n", "drwxrwxr-x",
					"linux", "linux", (long) 24001, name);
		}
	}
	freeReplyObject(reply);

	reply = redisCommand(context, "LRANGE FOLDER_%lu_files   0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			long id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFileName(context, id, name, MAX_FILENAME_SIZE);
			sprintf(line + strlen(line),
					"%s   1 %s %s %10lu Jan  1  1980 %s\r\n", "-rwxrwxr-x",
					"linux", "linux", (long) 24001, name);
		}
	}
	freeReplyObject(reply);

	return line;
}

void __mvDir(redisContext *context, long id, long newParentId){

}

void __mvFile(redisContext *context, char *fileId, long newParentId, char *newFileName){

}

//FIXME: return error codes
void vfs_mv(redisContext *context, char *oldPath, char *newpath){

	char buffer[MAX_FILENAME_SIZE];
	//take the name from the end

	vfs_getDirIdFromPath(context, oldPath);
	vfs_getFileIdFromPath(context, oldPath);

	//get the old parent from the rest of the path

	//now mv the file using that info
}

void vfs_mvDir(redisContext *context, char *oldPath, char *newpath){

}

void vfs_mvFile(redisContext *context, char *oldPath, char *newpath){

}

int main(int argc, char const *argv[]) {
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

	vfs_buildDatabase(c);
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
	tempId = vfs_getFileIdFromPath(c, "/");
	printf("idFromPath: %s, id: %ld\n", "/", tempId);
	char buffer[10000];
	vfs_getDirPathFromId(c, (long) 0, buffer, 10000);
	printf("FolderPathFromId: %lu , path: \"%s\"\n", (long) 0, buffer);
	//pretty print the files and folders
	//list the parts !!
	printf("calling pwd now\n");
	printf("%s", vfs_listUnixStyle(c, 0));
	vfs_ls(c, newDirId);
	long newIdS = vfs_getFileIdFromRelPath(c, 1, "foldhere");
	printf("%lu\n", newIdS);
	return 0;
}

