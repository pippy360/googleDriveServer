#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "./hiredis/hiredis.h"
#include "vfs.h"
#include "vfsPathParser.h"

#define MAX_FILENAME_SIZE 1000

void vfs_debug_printParserState(vfsPathParserState_t *parserState){
	printf("id's of oldpath %ld - %ld\n", parserState->parentId, parserState->id);
	printf("id                  %ld\n", parserState->id);
	printf("parent Id           %ld\n", parserState->parentId);
	printf("isFile              %d\n", parserState->isFile);
	printf("isDir               %d\n", parserState->isDir);
	printf("nameLength          %d\n", parserState->nameLength);
	printf("Existing Object:      %d\n", parserState->isExistingObject);
}

void init_vfsPathParserState(vfsPathParserState_t *parserState){
	parserState->error = 0;
	parserState->id = -1;
	parserState->isDir = 0;
	parserState->isFile = 0;
	parserState->isValid = 0;
	parserState->nameLength = 0;
	parserState->nameOffset = 0;
	parserState->parentId = -1;
	parserState->isExistingObject = 1;
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
//works for '.' and '..'
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
		for(i = 0; currPtr < path+pathLength && *currPtr != '/' ; i++){
			currPtr++;
		}
		if(currPtr >= path+pathLength){
			break;
		}
		currPtr++;//skip over the '/'

		result = vfs_findObjectInDir(context, &parserState, currDir, nameStart, i);
		printf("current name test %d %.*s\n", i, i, nameStart);
		if( result != 0 || parserState.id == -1 || !parserState.isDir){
			printf("invalid path, one of the tokens was not a directory\n");
			return -1;
		}
		currDir = parserState.id;
	}

	return currDir;
}

//takes in an empty vfsPathParserState_t
int vfs_parsePath(redisContext *context, vfsPathParserState_t *parserState, char *fullPath,
		int fullPathLength, long clientCwd) {

	//FIXME: handle 0 strings
	if(fullPathLength <= 0){
		//stuff
		return -1;
	}

	printf("the path: --%s--\n", fullPath);

	long cwd = (*fullPath == '/') ? ROOT_FOLDER_ID : clientCwd; //if it's not a relative path set CWD to ROOT_FOLDER_ID
	long tempId;
	int isLastCharSlash, fixedPathLength, i;
	char *tempPathPtr, *formattedPath ;

	//wipe the parsing state
	init_vfsPathParserState(parserState);

	//preprocess string
	//TODO: remove all the extra '///////'

/*
	//remove all the './'
	formattedPath = malloc(fullPathLength+1);//FIXME: it's a pain to free this with all the return functions we have!
	*formattedPath = '\0';
	tempPathPtr = formattedPath;
	for(i = 0; i+1 < fullPathLength; i++){
		if(strncmp(fullPath+i, "./", 2) == 0){
			i += 1;
		}else{
			sprintf(tempPathPtr, "%c", *(fullPath+i));
			tempPathPtr++;
		}
	}
	printf("the formatted path: --%s--\n", formattedPath);
*/

	//remove the last '/' if it has one
	isLastCharSlash = ( *(fullPath + fullPathLength - 1) == '/' );
	fixedPathLength = (isLastCharSlash)? fullPathLength-1 : fullPathLength;//cut the last '/' to make processing easier


	//special case for root, '/' or '/././' or '///'
	//if the string is empty and our CWD is root then
	//FIXME: this won't work for '/././'
	if(cwd == ROOT_FOLDER_ID && fixedPathLength == 0){
		parserState->id = ROOT_FOLDER_ID;
		parserState->parentId = ROOT_PARENT_ID;
		parserState->isDir = 1;
		parserState->isFile = 0;
		return 0;
	}

	//from this point on the string is '/etc' '/a/b.txt' 'a/b'

	//get all characters after the last '/',
	//After preprocessing and checking if root there will always be characters in the name
	//the name could be ".."
	if(vfs_serperatePathAndName(parserState, fullPath, fixedPathLength) != 0){
		printf("failed to get the last part of path, stuff after last '/'\n");
	}

	if((tempId = vfs_getDirIdFromPath(context, cwd, fullPath, fullPathLength)) == -1){
		printf("failed to get the get parent id\n");
		return -1;
	}

	if(parserState->nameLength > 0){
		printf("testing the name %s\n", fullPath+parserState->nameOffset);
		if(vfs_findObjectInDir(context, parserState, tempId, fullPath+parserState->nameOffset, parserState->nameLength) != 0){
			printf("failed to find in directory\n");
			//REMOVE:
			printf("about to test the existing object\n");
			parserState->isExistingObject = 0;
			parserState->parentId = tempId;
			printf("got it existing object\n");
		}else{
			parserState->isExistingObject = 1;
			if(parserState->isFile == 1)
				parserState->parentId = tempId;
			else{
				//parserState->parentId = tempId;//the parent might not be tempId, like if name == '..'
				printf("let's get the dir parent\n");
				parserState->parentId = vfs_getDirParent(context, parserState->id);
				printf("got the dir parent\n");
			}
		}
	}else{
		printf("The name length was zero???\n");
	}
	return 0;
}



int vfs_mv(redisContext *context, long cwd, char *oldPath, char *newPath){
	vfsPathParserState_t oldPathParserState, newPathParserState;
	printf("done with none\n");
	int result1 = vfs_parsePath(context, &oldPathParserState, oldPath, strlen(oldPath), cwd);
	printf("done with the first one\n");
	int result2 = vfs_parsePath(context, &newPathParserState, newPath, strlen(newPath), cwd);

	printf("\noldpath %s\n", oldPath);
	printf("name:               %.*s\n", oldPathParserState.nameLength, oldPath + oldPathParserState.nameOffset);
	vfs_debug_printParserState(&oldPathParserState);

	printf("\nnewpath %s\n", newPath);
	printf("name:               %.*s\n", newPathParserState.nameLength, newPath + newPathParserState.nameOffset);
	vfs_debug_printParserState(&newPathParserState);

	//check if it's a valid mv
	if(oldPathParserState.isExistingObject == 0){
		printf("error source file doesn't exist\n");
		return -1;
	}

	if(newPathParserState.isExistingObject){
		if(oldPathParserState.isFile != newPathParserState.isFile){
			printf("error file/dir not the same type!!\n");
			return -1;
		}
	}else{
		if( *(newPath + strlen(newPath) -1) == '/' ){
			//CAN   mv ./test/ ./here/ if here doesn't exist
			//CAN'T mv ./test.txt ./here/ if here doesn't exist
			if(oldPathParserState.isDir){
				newPathParserState.isFile = 0;
				newPathParserState.isDir  = 1;
			}else{
				printf("error directory doesn't exist\n");
				return -1;
			}
		}else{
			newPathParserState.isFile = oldPathParserState.isFile;
			newPathParserState.isDir  = oldPathParserState.isDir;
		}
	}

	if(newPathParserState.isExistingObject && oldPathParserState.id == newPathParserState.id){
		//mv /a/something.txt /a/../a/something.txt
		//do nothing
	}else if(oldPathParserState.isFile && newPathParserState.isExistingObject && newPathParserState.isFile){
		//overwrite file
		printf("overwrite file\n");
		__removeIdFromFileList(context, newPathParserState.parentId, newPathParserState.id);
		__removeIdFromFileList(context, oldPathParserState.parentId, oldPathParserState.id);

		__addFileToFileList(context, newPathParserState.parentId, oldPathParserState.id);
	}else if(oldPathParserState.isFile && !newPathParserState.isExistingObject && newPathParserState.isDir){
		//rename file
		printf("rename file\n");
		__removeIdFromFileList(context, oldPathParserState.parentId, oldPathParserState.id);

		__addFileToFileList(context, newPathParserState.parentId, oldPathParserState.id);
	}else if(oldPathParserState.isFile && !newPathParserState.isExistingObject && newPathParserState.isDir){
		//move file to existing dir
		printf("move file to existing dir\n");
		__removeIdFromFileList(context, oldPathParserState.parentId, oldPathParserState.id);

		__addFileToFileList(context, newPathParserState.id, oldPathParserState.id);
	}else if(oldPathParserState.isDir && newPathParserState.isExistingObject && newPathParserState.isDir){
		//overwrite dir
		printf("overwrite dir\n");
		__removeIdFromFolderList(context, oldPathParserState.parentId, oldPathParserState.id);
		__removeIdFromFolderList(context, newPathParserState.parentId, newPathParserState.id);

		__addDirToFolderList(context, newPathParserState.parentId, oldPathParserState.id);
		vfs_setDirParent(context, oldPathParserState.id, newPathParserState.parentId);
	}else if(oldPathParserState.isDir && !newPathParserState.isExistingObject && newPathParserState.isDir){
		//rename dir
		printf("rename dir\n");
		__removeIdFromFolderList(context, oldPathParserState.parentId, oldPathParserState.id);

		__addDirToFolderList(context, newPathParserState.parentId, oldPathParserState.id);
		vfs_setDirParent(context, oldPathParserState.id, newPathParserState.parentId);
	}else{
		printf("you'll get here if you try to move a file to a dir that doens't exist and possibly for other reasons\n");
	}

	return 0;
}











