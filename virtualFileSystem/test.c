
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

	vfsPathHTTPParserState_t parserState, parserState2;
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

	 vfsPathHTTPParserState_t o, n;
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