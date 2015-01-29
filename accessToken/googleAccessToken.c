#include <stdio.h>
#include <stdlib.h>
#include "net/networking.h"
#include "parser.h"

//FIXME: these can be a VERY expensive functions to execute
//this whole page is a minefield, there's hdd access, send/recv'ing and printf/fget'ing

#define CODE_REQUEST_POST_DATA "code=%s&"\
		"client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps.googleusercontent.com&"\
		"client_secret=BDkeIsUA394I_n-hwFp6syeO&"\
		"redirect_uri=urn:ietf:wg:oauth:2.0:oob&"\
		"grant_type=authorization_code"
#define REFRESH_TOKEN_DATA "client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu."\
		"apps.googleusercontent.com&"\
		"client_secret=BDkeIsUA394I_n-hwFp6syeO&&"\
		"refresh_token=%s&"\
		"grant_type=refresh_token"

//returns 0 if success, -1 if error where errorno is set, -2 otherwise
//puts a '\0' terminated string in accessTokenBuffer
//it can return an invalid acces token
int loadFromFile(void *accessTokenBuffer, const int bufferLength){
	char * line = NULL;
	size_t zero = 0;
	FILE *f = fopen("refresh.token", "r");
	if (f == NULL)
		return -1;
	
	rewind(f);
	int length = getline(&line, &zero, f);
	if(length > bufferLength)
		return -2;

	memcpy(accessTokenBuffer, line, length);
	//remove the '\n'
	accessTokenBuffer[length-1] = '\0';//FIXME: handle windows line ending!!!

	free(line);
	fclose(f);
	return 0;
}

void saveToFile(const char *accessTokenBuffer, const int accessTokenLength){
	FILE *f = fopen("refresh.token", "w");
	fprintf(f, "%.*s\n", accessTokenLength, accessTokenBuffer);
	fclose(f);
}

//does all the connect, fetch file and parse
//set any token you don't want to null
void getFromGoogle(char *jsonFileUrl, AccessTokenState_t *stateStruct){
    /*
    *
    *
    *USE THIS
    *
    *
    */
}

//returns 0 if success, -1 if error where errorno is set, -2 otherwise
int init_googleAccessToken(AccessTokenState_t *stateStruct, void *buffer, 
							const int bufferLength, int *stateStructSize){
	int error;
	
	//load it straight into the struct
	if((error = loadFromFile(buffer, bufferLength)) != 0){
		return error;
	}

	//try to use the refresh token to get an access token
	error = getAccessTokenWithRefreshToken(buffer void* accessTokenBuffer, 
							const int bufferLength, int *accessTokenLength);
	if(error != 0){
		//if it fails then get a refresh token
		//save it
		//try get an accesstoken again, if that fails keep asking for input accesstoken
		return error;
	}

	//now we have an access/refresh token, add it to the stateStruct
	//and build the stateStruct
	//set the size of the state struct
	//write a build stateStruct function
}

//refreshTokenStr must be null terminated
int getAccessTokenWithRefreshToken(const char *refreshTokenStr){

	//go off to google and get the token
	//parse it using getJsonValue()
}

//codeStr must be null terminated
char* getAccessTokenWithPastedCode(const char *codeStr, void* accessTokenBuffer, 
									const int bufferLength, int *accessTokenLength){
	//go off to google and get the token
	//parse it using getJsonValue()

}

//TODO: ERROR CODES
int getNewTokensWithLink(void* accessTokenBuffer , const int accessTokenBufferLength, 
						 void* refreshTokenBuffer, const int refreshTokenBufferLength, 
						int *accessTokenLength){
	char buffer[MAX_RETURN_CODE_LENGTH + 1];

	printf("Visit this link and paste the code in...:\n"
		"https://accounts.google.com/o/oauth2/auth?access_type=offline"
		"&client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps."
		"googleusercontent.com&redirect_uri=urn%%3Aietf%%3Awg%%3Aoauth"
		"%%3A2.0%%3Aoob&response_type=code&scope=https%%3A%%2F%%2Fwww."
		"googleapis.com%%2Fauth%%2Fdrive\n");

	fgets(buffer, MAX_RETURN_CODE_LENGTH, stdin);
	buffer[ strlen(buffer) - 1 ] = '\0';//cut out the last char
	getAccessTokenWithPastedCode( buffer );
}

void getAccessToken(AccessTokenState_t *stateStruct, void* buffer, int *accessTokenLength){

	//check if the access token is valid
	if ( ATExpireTime > (unsigned)time(NULL) ){
		return currentAccessToken;
	}else if( currentRefreshToken != NULL ){
		char* output = getAccessTokenWithRefreshToken( currentRefreshToken );
		//if there's a bad refresh token
		if( output == NULL )
			output = getNewAccessTokenWithLink();

		saveToFile();
		return output;
	}else{
		//else ask for the user to get one
		char* output = getNewAccessTokenWithLink();
		saveToFile();
		return output;
	}
}

