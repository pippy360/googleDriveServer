#include "vfsPathParser.h"

#define MAX_FILENAME_SIZE 1000



//finds parserState->namePtr
//returns 0 if success, non-0 otherwise
int vfs_serperatePathAndName(vfsPathParserState_t *parserState, char *fullFilePath,
		int fullFilePathLength) {

	int i = 0;
	int containsFileName;//boolean, does it end in a '/' or a file name
	char *currPos = fullFilePath+fullFilePathLength-1;

	for(i = 0; currPos > fullFilePath && *currPos != '/'; i++){
		currPos--;
	}
	if(currPos <= fullFilePath){
		return -1;
	}
	parserState->nameLength = i;
	parserState->namePtr = currPos;
	return 0;
}

//this finds a "name" in a directory (either a file or a folder)
//and it can also tell us if it's a file or a folder
int vfs_findInDir(vfsPathParserState_t *parserState, long dirId, char *name,
		int nameLength) {
	/*
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
	*/
	return 0;
}

//the path should start at the name of the first folder, so "/fold/" should be "fold/"
int vfs_getParentId(vfsPathParserState_t *parserState, long cwd, char *path,
		int pathLength) {
	vfsPathParserState_t tempState;
	int i, nameLength;
	char *currChar, *nameStart;
	long currCwd;

	nameStart = path;
	while(/*not at the end*/){//loop for each name

		for(i = 0; *currChar != '/'; i++, currChar++)//loop for each character
			;

		currChar++;//skip over the '/'
		nameLength = currChar - nameStart - 1;

		//FIXME: handle errors
		vfs_findInDir(tempState, currCwd, currChar, nameLength);

		//FIXME: you need to make sure it's a folder
		cwd = tempState->id;
		nameStart = currChar;
	}
	parserState->parentId = tempState->id;
}

int vfs_getDirId(long cwd, char *path, int pathLength){

}

//takes in an empty parser state
int vfs_parsePath(vfsPathParserState_t *parserState, char *fullPath,
		int fullPathLength, long clientCwd) {

	if(fullPathLength == 0){
		return -1;
	}
	long cwd = (*fullPath == '/') ? ROOT_FOLDER_ID : clientCwd; //if it's not a relative path set CWD to ROOT_FOLDER_ID

	if(vfs_serperatePathAndName(parserState, fullPath, fullPathLength) != 0){
		printf("failed to get the last part of path, stuff after last '/'\n");
	}
	if(vfs_getParentId(parserState, cwd, fullPath, fullPathLength) != 0){
		printf("failed to get the get parent id\n");
	}

	if(parserState->nameLength > 0){
		if(vfs_findInDir(parserState, parserState->parentId, parserState->namePtr,
				parserState->nameLength) != 0){
			printf("failed\n");
		}
	}
	return 0;
}

