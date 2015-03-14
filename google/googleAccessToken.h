
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

void gat_newAccessTokenState(AccessTokenState_t *stateStruct);

void gat_freeAccessTokenState(AccessTokenState_t *stateStruct);

int gat_init_googleAccessToken(AccessTokenState_t *stateStruct);

void gat_getAccessToken(AccessTokenState_t *stateStruct);

