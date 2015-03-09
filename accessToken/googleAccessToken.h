
typedef struct {
	char 	isAccessTokenLoaded;//boolean
	char* 	accessTokenStr;//'\0' terminated 
	int 	accessTokenBufferLength;//the length of the buffer,-
									//-if an access token is greater than- 
									//-this then realloc will have to be called 
									//IT IS NOT THE LENGTH OF THE ACCESS TOKEN!! 
	int 	expireTime;
	char* 	refreshTokenStr;//'\0' terminated
	int 	refreshTokenBufferLength;
} AccessTokenState_t;


void init_googleAccessToken(AccessTokenState_t *stateStruct);

void getAccessToken(AccessTokenState_t *stateStruct, void* buffer, int *accessTokenLength);