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

void __deleteFile(redisContext *context, long fileId){

}

void __deleteDir(redisContext *context, long dirId){

}

//FIXME: return error codes
void vfs_delete(redisContext *context, char *path){

}

void __mvDir(redisContext *context, long id, long newParentId){

}

void __mvFile(redisContext *context, char *fileId, long newParentId, char *newFileName){

}

//FIXME: return error codes
void vfs_mv(redisContext *context, char *oldPath, char *newpath){

	char buffer[MAX_FILENAME_SIZE];
	//take the name from the end


	//get the old parent from the rest of the path

	//now mv the file using that info
}

//  ####  ###### ##### ##### ###### #####   ####
// #    # #        #     #   #      #    # #
// #      #####    #     #   #####  #    #  ####
// #  ### #        #     #   #      #####       #
// #    # #        #     #   #      #   #  #    #
//  ####  ######   #     #   ###### #    #  ####


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

//return's id if successful, -1 otherwise
long vfs_findFileNameInDir(redisContext *context, long dirId, char *fileName, int fileNameLength){
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];
	printf("LRANGE FOLDER_%lu_files 0 -1\n", dirId);
	reply = redisCommand(context, "LRANGE FOLDER_%lu_files 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			tempId = strtol(reply->element[i]->str, NULL, 10);
			vfs_getFileName(context, tempId, nameBuffer, MAX_FILENAME_SIZE);
			printf("nameBuffer %s\n", nameBuffer);
			if(strncmp(fileName, nameBuffer, fileNameLength) == 0){
				matchId = tempId;
			}
		}
	}
	freeReplyObject(reply);
	return matchId;
}

//return's id if successful, -1 otherwise
long vfs_findDirNameInDir(redisContext *context, long dirId, char *dirName, int dirNameLength){
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			tempId = strtol(reply->element[i]->str, NULL, 10);
			vfs_getFolderName(context, tempId, nameBuffer, MAX_FILENAME_SIZE);
			if(strncmp(dirName, nameBuffer, dirNameLength) == 0){
				matchId = tempId;
			}
		}
	}
	freeReplyObject(reply);
	return matchId;
}

//FIXME: SO MUCH REPEATED CODE HERE, CLEAN THIS UP
//-1 if not found, id otherwise


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

#include "vfsPathParser.h"

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
	tempId1 = vfs_findDirNameInDir (c, 0, "new folder", strlen("new folder"));
	printf("The id %ld\n", tempId1);

	printf("lets find foldhere\n");
	tempId3 = vfs_findFileNameInDir(c, 0, "foldhere", strlen("foldhere"));
	printf("The id %ld\n", tempId3);
	tempId1 = vfs_findDirNameInDir (c, 0, "new folder", strlen("new folder"));
	printf("The id %ld\n", tempId1);
	printf("let's print the /new folder/\n");
	vfs_ls(c, tempId1);
	tempId3 = vfs_findDirNameInDir(c, tempId1, "foldhere", strlen("foldhere"));
	printf("The id %ld\n", tempId3);

	tempId1 = vfs_findFileNameInDir(c, 0, "far_down_file.webm", strlen("far_down_file.webm"));
	printf("The id %ld\n", tempId1);

	printf("let's print the /new folder/foldhere/\n");
	vfs_ls(c, tempId3);

	tempId1 = vfs_findFileNameInDir(c, tempId3, "far_down_file.webm", strlen("far_down_file.webm"));
	printf("The id %ld\n", tempId1);
	vfsPathParserState_t parserState;
	init_vfsPathParserState(&parserState);
	vfs_findObjectInDir(c, &parserState, tempId3, "far_down_file.webm", strlen("far_down_file.webm"));
	printf("is it a file??? %d\n", parserState.isFile);

	char *str1 = "sadf/";
	vfs_serperatePathAndName(&parserState, str1, strlen(str1));
	printf("the last bit of %s - %d %s\n", str1, parserState.nameLength, str1 + parserState.nameOffset);


	init_vfsPathParserState(&parserState);
	char *tempPath = "/new folder/foldhere/sdfsdf/";
	printf("lets get the dir id using a path\n");
	printf("the path %s\n", tempPath);
	printf("tempId3 before %ld\n", tempId3);
	tempId3 = vfs_getDirIdFromPath(c, 0, tempPath, strlen(tempPath));
	printf("tempId3 after %ld\n", tempId3);
	vfs_findObjectInDir(c, &parserState, tempId3, "far_down_file.webm", strlen("far_down_file.webm"));
	printf("is it a file??? %d\n", parserState.isFile);

	return 0;
}

