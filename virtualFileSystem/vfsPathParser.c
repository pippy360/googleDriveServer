#include "./hiredis/hiredis.h"
#include "vfsPathParser.h"

#define MAX_FILENAME_SIZE 1000



void init_vfsPathParserState(vfsPathParserState_t *parserState){
	parserState->error = 0;
	parserState->id = -1;
	parserState->isDir = 0;
	parserState->isFile = 0;
	parserState->isValid = 0;
	parserState->nameLength = 0;
	parserState->nameOffset = 0;
	parserState->parentId = -1;
}

//finds parserState->namePtr
//returns 0 if success, non-0 otherwise
int vfs_serperatePathAndName(vfsPathParserState_t *parserState, char *fullFilePath,
		int fullFilePathLength) {

	int i = 0;
	int containsFileName;//boolean, does it end in a '/' or a file name
	char *currPos = fullFilePath+fullFilePathLength-1;

	for(i = 0; currPos >= fullFilePath && *currPos != '/'; i++){
		currPos--;
	}
	if(currPos < fullFilePath){
		parserState->nameLength = fullFilePathLength;
		parserState->nameOffset = 0;
	}else{
		parserState->nameLength = i;
		parserState->nameOffset = (currPos+1)-fullFilePath;
	}

	return 0;
}

//this finds a "name" in a directory (either a file or a folder),
//fills in the id and object type (isDir/isFile) in the parserState
//returns 0 if success, non-0 otherwise
int vfs_findObjectInDir(redisContext *context, vfsPathParserState_t *parserState, long dirId, char *name,
		int nameLength) {
	long resultId;
	int isFile = 0, isDir = 0;
	if((resultId = vfs_findFileNameInDir(context, dirId, name, nameLength)) != -1){
		isFile = 1;
	}else if((resultId = vfs_findDirNameInDir(context, dirId, name, nameLength)) != -1){
		isDir = 1;
	}
	if(resultId == -1){
		return -1;
	}

	parserState->id 		= resultId;
	parserState->isFile 	= isFile;
	parserState->isDir 		= isDir;

	return 0;
}

//will follow the string until the last '/' and will ignore any characters after that
long vfs_getDirIdFromPath(redisContext *context, long userCwd, char *path, int pathLength){
	char *currPtr = path, *nameStart;
	int i = 0, result;
	long currDir = userCwd;
	vfsPathParserState_t parserState;
	init_vfsPathParserState(&parserState);

	if(*currPtr == '/'){
		currPtr++;
		currDir = 0;//FIXME: REPLACE WITH ROOTid
	}
	while(1){
		nameStart = currPtr;
		printf("let's loop %s\n", currPtr);
		for(i = 0; currPtr < path+pathLength && *currPtr != '/' ; i++){
			printf("let's loop %c\n", *currPtr);
			currPtr++;
		}
		if(currPtr >= path+pathLength){
			break;
		}
		currPtr++;//skip over the '/'

		printf("the name we're looking for %d %.*s\n", i, i, nameStart);
		printf("it's in %ld\n", currDir);
		result = vfs_findObjectInDir(context, &parserState, currDir, nameStart, i);
		if( result != 0 || parserState.id == -1 || !parserState.isDir){
			printf("invalid path, one of the tokens was not a directory\n");
			return -1;
		}
		currDir = parserState.id;
	}

	return currDir;
}

//takes in an empty parser state
int vfs_parsePath(redisContext *context, vfsPathParserState_t *parserState, char *fullPath,
		int fullPathLength, long clientCwd) {

	if(fullPathLength == 0){
		return -1;
	}
	long cwd = (*fullPath == '/') ? ROOT_FOLDER_ID : clientCwd; //if it's not a relative path set CWD to ROOT_FOLDER_ID

	if(vfs_serperatePathAndName(parserState, fullPath, fullPathLength) != 0){
		printf("failed to get the last part of path, stuff after last '/'\n");
	}
	if(vfs_getDirIdFromPath(context, cwd, fullPath, fullPathLength) != 0){
		printf("failed to get the get parent id\n");
	}

	if(parserState->nameLength > 0){
//		if(vfs_findInDir(parserState, parserState->parentId, parserState->namePtr,
//				parserState->nameLength) != 0){
//			printf("failed\n");
//		}
	}
	return 0;
}

