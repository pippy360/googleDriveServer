#define ROOT_FOLDER_ID 0
#define ROOT_PARENT_ID -1

typedef struct {
	char isExistingObject;
	char nameOffset;//offset points to any characters after the last '/' in the file path
	int nameLength;
	//@isFilePath If this is true it DOESN'T mean it's an existing file,
	//only that it's a file path (like "/something.txt as opposed to a directory path like /something/
	char isFilePath;
	char isDir;//FIXME: REPLACE WITH ENUMS (objectType)
	char isValid;
	int error;
	//parentId: if parsing a path that points to a file "/something/here.txt",
	//then the parentId is the id of the folder the file is in
	//parentId: if parsing a path that points to folder "/something/here/ or /etc",
	//then the parentId is the id of the parent folder
	long parentId;
	long id;//id of the file/folder the path points to
} vfsPathParserState_t;

