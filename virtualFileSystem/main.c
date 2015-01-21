#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./hiredis/hiredis.h"

void init(){

}

long getNewId(redisContext *context){
    redisReply *reply;
    reply = redisCommand(context, "GET current_id_count");
    long result = strtol(reply->str, NULL, 10);
    freeReplyObject(reply);
    reply = redisCommand(context, "INCR current_id_count");
    freeReplyObject(reply);
    return result;
}

void __mkdir(redisContext *context, long parentId, long id, char *name){
    redisReply *reply;
    reply = redisCommand(context, "HMSET FOLDER_%lu_info  parent \"%lu\" name \"%s\" createdData \"%s\"", 
                        id, parentId, name, "march 7th");
    freeReplyObject(reply);
}

//id 0 == root
void buildDatabase(redisContext *context){
	//wipe it and set the first id to 0 and crate a root folder
    redisReply *reply;
	reply = redisCommand(context, "FLUSHALL");
    freeReplyObject(reply);
	__mkdir(context, 0, 0, "root");
	reply = redisCommand(context, "SET current_id_count 0");
    freeReplyObject(reply);
}

//returns dir id
long vfs_mkdir(redisContext *context, long parentId, char *name){
	long id = getNewId(context);
    redisReply *reply;
    reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", parentId, id);
    freeReplyObject(reply);
    __mkdir(context, parentId, id, name);
    return id;
}

void __createFile(redisContext *context, long id, char *name, long size){
    redisReply *reply;
    reply = redisCommand(context, "HMSET FILE_%lu_info name \"%s\" size \"%lu\" createdData \"%s\" ", 
                            id, name, size, "march 7th");
    freeReplyObject(reply);
}

long vfs_createFile(redisContext *context, long parentId, char *name, long size){
	//add it to the file list of the dir
	long id = getNewId(context);
    redisReply *reply;
    reply = redisCommand(context, "LPUSH FOLDER_%lu_files %lu", parentId, id);
    freeReplyObject(reply);
    __createFile(context, id, name, size);
    return id;
}

//return part id
void vfs_createPart(redisContext *context, long fileId, char *partUrl, long rangeStart, long rangeEnd){
	//add it to the list
}

void __createPart(){
	//creates the info part
}

void vfs_pwd(redisContext *context, long dirId){
    int j = 0;
    redisReply *reply;
    reply = redisCommand(context,"LRANGE FOLDER_%lu_folders 0 -1", dirId);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%s\n", reply->element[j]->str);
        }
    }
    freeReplyObject(reply);
    reply = redisCommand(context,"LRANGE FOLDER_%lu_files   0 -1", dirId);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%s\n", reply->element[j]->str);
        }
    }
    freeReplyObject(reply);
}

void rm_dir(){

}

void rm_file(){
	
}

int main(int argc, char const *argv[])
{
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

    buildDatabase(c);
    long newDirId = vfs_mkdir(c, 0, "i'm in root !!");
    long newFileId = vfs_createFile(c, newDirId, "a_new_file.webm", 1000);

    vfs_pwd(c, 0);
    //pretty print the files and folders
    //list the parts !!

    return 0;
}

