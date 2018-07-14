#include "vfsPathParser.h"

#define MAX_FILENAME_SIZE 1000

void vfs_debug_printParserState(vfsPathHTTPParserState_t *parserState) {
	printf("id's of oldpath %ld - %ld\n", parserState->fileObj.parentId,
			parserState->fileObj.id);
	printf("id                  %ld\n", parserState->fileObj.id);
	printf("parent Id           %ld\n", parserState->fileObj.parentId);
	printf("isFile              %d\n", parserState->isFilePath);
	printf("isDir               %d\n", parserState->fileObj.isDir);
	printf("nameLength          %d\n", parserState->nameLength);
	printf("Existing Object:      %d\n", parserState->isExistingObject);
}

void init_vfsPathParserState(vfsPathHTTPParserState_t *parserState) {
	parserState->error = 0;
	parserState->isFilePath = 0;
	parserState->isValidPath = 0;
	parserState->nameLength = 0;
	parserState->nameOffset = 0;
	parserState->fileObj.isDir = 0;
	parserState->fileObj.id = -1;
	parserState->fileObj.parentId = -1;
	parserState->isExistingObject = 1;
}

//finds parserState->namePtr
//returns 0 if success, non-0 otherwise
int __vfs_serperatePathAndName(vfsPathHTTPParserState_t *parserState,
		const char *fullFilePath, int fullFilePathLength) {

	int i = 0;
	const char *currPos = fullFilePath + fullFilePathLength - 1;

	for (i = 0; currPos >= fullFilePath && *currPos != '/'; i++) {
		currPos--;
	}
	if (currPos < fullFilePath) {
		parserState->nameLength = fullFilePathLength;
		parserState->nameOffset = 0;
	} else {
		parserState->nameLength = i;
		parserState->nameOffset = (currPos + 1) - fullFilePath;
	}

	return 0;
}

//this finds a "name" in a directory (either a file or a folder),
//fills in the id and object type (isDir/isFile) in the parserState
//returns 0 if success, non-0 otherwise
//works for '.' and '..'
int __vfs_findObjectInDir( redisContext *context, vfsPathHTTPParserState_t *parserState, 
		long dirId, const char *name, int nameLength ) {

	long resultId;
	int isFile = 0, isDir = 0;

	if ( ( resultId = vfs_findFileNameInDir( 
			context, dirId, name, nameLength ) ) != -1) {

		isFile = 1;
	} else if ( ( resultId = vfs_findDirNameInDir( 
			context, dirId, name, nameLength ) ) != -1 ) {

		isDir = 1;
	}
	if ( resultId == -1 ) {
		return -1;
	}

	parserState->fileObj.id = resultId;
	parserState->isFilePath = isFile;
	parserState->fileObj.isDir = isDir;

	return 0;
}

long getLengthOfNextName(const char *remainingPath, long remainingPathLength){
	int i;
	for (i = 0; i < remainingPathLength && remainingPath[i] != '/'; i++) {
		;
	}
	return i;
}

//will follow the string until the last '/' and will ignore any characters after that
long vfs_getDirIdFromPath(redisContext *context, long userCwd, const char *path,
		int pathLength) {
	const char *currPtr = path, *nameStart;
	int result;
	long currDir = userCwd;
	vfsPathHTTPParserState_t parserState;
	init_vfsPathParserState(&parserState);

	if (*currPtr == '/') {
		currPtr++;
		currDir = 0; //FIXME: REPLACE WITH ROOTid
	}
	while (1) {
		nameStart = currPtr;
		long remainingLength = pathLength - (currPtr-path);
		long lengthOfNextName = getLengthOfNextName(currPtr,
				remainingLength);
		currPtr += lengthOfNextName;
		if (currPtr >= path + pathLength) {
			break;
		}
		currPtr++; //skip over the '/'

		result = __vfs_findObjectInDir(context, &parserState, currDir, nameStart,
				lengthOfNextName);
		if (result != 0 || parserState.fileObj.id == -1 || !parserState.fileObj.isDir) {
			printf("invalid path, one of the tokens was not a directory\n");
			return -1;
		}
		currDir = parserState.fileObj.id;
	}

	return currDir;
}

//takes in an empty vfsPathHTTPParserState_t
int __vfs_parsePath(redisContext *context, vfsPathHTTPParserState_t *parserState,
		const char *fullPath, int fullPathLength, long clientCwd) {

	//FIXME: handle 0 strings
	if (fullPathLength <= 0) {
		//stuff
		return -1;
	}

	long cwd = (*fullPath == '/') ? ROOT_FOLDER_ID : clientCwd; //if it's not a relative path set CWD to ROOT_FOLDER_ID
	long tempId;
	int isLastCharSlash, fixedPathLength;

	//wipe the parsing state
	init_vfsPathParserState(parserState);

	//preprocess string
	//TODO: remove all the extra '///////'

	//remove the last '/' if it has one
	isLastCharSlash = (*(fullPath + fullPathLength - 1) == '/');
	fixedPathLength = (isLastCharSlash) ? fullPathLength - 1 : fullPathLength;//cut the last '/' to make processing easier

	//special case for root, '/' or '/././' or '///'
	//if the string is empty and our CWD is root then
	//FIXME: this won't work for '/././'
	if (cwd == ROOT_FOLDER_ID && fixedPathLength == 0) {
		parserState->fileObj.id = ROOT_FOLDER_ID;
		parserState->fileObj.parentId = ROOT_PARENT_ID;
		parserState->fileObj.isDir = 1;
		parserState->isFilePath = 0;
		parserState->isValidPath = 1;
		return 0;
	}

	//from this point on the string is '/etc' '/a/b.txt' 'a/b'

	//get all characters after the last '/',
	//After preprocessing and checking if root there will always be characters in the name
	//the name could be ".."
	if (__vfs_serperatePathAndName(parserState, fullPath, fixedPathLength) != 0) {
		printf("ERROR: failed to get the last part of path, stuff after last '/'\n");
	}

	if ((tempId = vfs_getDirIdFromPath(context, cwd, fullPath, fixedPathLength))
			== -1) {
		printf("ERROR: failed to get the get parent id\n");
		return -1;
	}

	if (parserState->nameLength > 0) {
		if (__vfs_findObjectInDir(context, parserState, tempId,
				fullPath + parserState->nameOffset, parserState->nameLength)
				!= 0) {
			parserState->isExistingObject = 0;
			parserState->fileObj.parentId = tempId;
		} else {
			parserState->isExistingObject = 1;
			if (parserState->isFilePath == 1)
				parserState->fileObj.parentId = tempId;
			else {
				parserState->fileObj.parentId = __vfs_getDirParent(context,
						parserState->fileObj.id);
			}
		}
	} else {
		printf("ERROR: The name length was zero???\n");
	}
	return 0;
}

int vfs_getIdByPath(redisContext *context, char *path, long cwd ) {
	vfsPathHTTPParserState_t parserState;
	if( __vfs_parsePath( context, &parserState, path, strlen(path), cwd ) ) {
		return -1;
	}
	
	return parserState.fileObj.id;

}

//returns 0 if success, non-0 otherwise
int __vfs_deleteObjectWithPath(redisContext *context, const char *path, long cwd){
	vfsPathHTTPParserState_t parserState;
	int rc = __vfs_parsePath(context, &parserState, path, strlen(path), cwd);
	if (!rc) {
		return rc;
	}
	if(!parserState.isExistingObject){
		return -1;
	}

	printf("removing path %ld %s\n", cwd, path);

	if(parserState.fileObj.isDir){
		//FIXME:
		printf("removing dir %ld %ld\n", parserState.fileObj.parentId, parserState.fileObj.id);
		vfs_removeIdFromFolderList(context, parserState.fileObj.parentId, parserState.fileObj.id);
	}else if(parserState.isFilePath){
		vfs_removeIdFromFileList(context, parserState.fileObj.parentId, parserState.fileObj.id);
	}
	return 0;
}

int __vfs_mv(redisContext *context, long cwd, const char *oldPath, 
		const char *newPath) {

	vfsPathHTTPParserState_t oldPathParserState, newPathParserState;
	int rc;
	rc = __vfs_parsePath(context, &oldPathParserState, oldPath,
			strlen(oldPath), cwd);
	if (!rc) {
		printf("FIXME : invalid path when calling mv \n");
		return rc;
	}
	rc = __vfs_parsePath(context, &newPathParserState, newPath,
			strlen(newPath), cwd);
	if (!rc) {
		printf("FIXME : invalid path when calling mv \n");
		return rc;
	}

	//check if it's a valid mv
	if (oldPathParserState.isExistingObject == 0) {
		printf("error source file doesn't exist\n");
		return -1;
	}

	if (newPathParserState.isExistingObject) {
		if (oldPathParserState.isFilePath != newPathParserState.isFilePath) {
			printf("error file/dir not the same type!!\n");
			return -1;
		}
	} else {
		if (*(newPath + strlen(newPath) - 1) == '/') {
			//CAN   mv ./test/ ./here/ if here doesn't exist
			//CAN'T mv ./test.txt ./here/ if here doesn't exist
			if (oldPathParserState.fileObj.isDir) {
				newPathParserState.isFilePath = 0;
				newPathParserState.fileObj.isDir = 1;
			} else {
				printf("error directory doesn't exist\n");
				return -1;
			}
		} else {
			newPathParserState.isFilePath = oldPathParserState.isFilePath;
			newPathParserState.fileObj.isDir = oldPathParserState.fileObj.isDir;
		}
	}

	if (newPathParserState.isExistingObject
			&& oldPathParserState.fileObj.id == newPathParserState.fileObj.id) {
		//mv /a/something.txt /a/../a/something.txt
		//do nothing
	} else if (oldPathParserState.isFilePath && newPathParserState.isExistingObject
			&& newPathParserState.isFilePath) {
		//overwrite file
		vfs_removeIdFromFileList(context, newPathParserState.fileObj.parentId,
				newPathParserState.fileObj.id);
		vfs_removeIdFromFileList(context, oldPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);

		vfs_addFileToFileList(context, newPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);
	} else if (oldPathParserState.isFilePath && !newPathParserState.isExistingObject
			&& newPathParserState.isFilePath) {
		//rename file
		vfs_removeIdFromFileList(context, oldPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);

		vfs_addFileToFileList(context, newPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);
		vfs_setFileName(context, oldPathParserState.fileObj.id,
				newPath + newPathParserState.nameOffset,
				newPathParserState.nameLength);
	} else if (oldPathParserState.isFilePath && !newPathParserState.isExistingObject
			&& newPathParserState.fileObj.isDir) {
		//move file to existing dir
		vfs_removeIdFromFileList(context, oldPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);

		vfs_addFileToFileList(context, newPathParserState.fileObj.id,
				oldPathParserState.fileObj.id);
	} else if (oldPathParserState.fileObj.isDir && newPathParserState.isExistingObject
			&& newPathParserState.fileObj.isDir) {
		//overwrite dir
		vfs_removeIdFromFolderList(context, oldPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);
		vfs_removeIdFromFolderList(context, newPathParserState.fileObj.parentId,
				newPathParserState.fileObj.id);

		vfs_addDirToFolderList(context, newPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);
		__vfs_setDirParent(context, oldPathParserState.fileObj.id,
				newPathParserState.fileObj.parentId);
	} else if (oldPathParserState.fileObj.isDir && !newPathParserState.isExistingObject
			&& newPathParserState.fileObj.isDir) {
		//rename dir
		vfs_removeIdFromFolderList(context, oldPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);

		vfs_addDirToFolderList(context, newPathParserState.fileObj.parentId,
				oldPathParserState.fileObj.id);
		__vfs_setDirParent(context, oldPathParserState.fileObj.id,
				newPathParserState.fileObj.parentId);
	} else {
		printf(
			"you'll get here if you try to move a file to a"
			" dir that doens't exist and possibly for other reasons\n");
	}

	return 0;
}


