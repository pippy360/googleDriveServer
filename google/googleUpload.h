#ifndef GOOGLEUPLOAD_H
#define GOOGLEUPLOAD_H

#include "../net/connection.h"

typedef struct {
	int didItWork;
	char *id;
	char *name;
	long fileSize;
	char *webUrl;
	char *apiUrl;//short lived link
} GoogleUploadState_t;

int googleUpload_init(Connection_t *con, AccessTokenState_t *accessTokenState,
		char *metadata, char *contentType);

int googleUpload_update(Connection_t *con, char *dataBuffer, int dataLength);

int googleUpload_end(Connection_t *con, GoogleUploadState_t *fileState);


#endif /* GOOGLEUPLOAD_H */
