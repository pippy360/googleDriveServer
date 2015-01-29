
typedef struct {
	char 	isAccessTokenLoaded;//boolean
	char* 	accessTokenStr;
	int 	expireTime;
	char* 	refreshTokenStr;
} AccessTokenState_t;


void init_googleAccessToken(AccessTokenState_t *stateStruct);

void getAccessToken(AccessTokenState_t *stateStruct, void* buffer, int *accessTokenLength);