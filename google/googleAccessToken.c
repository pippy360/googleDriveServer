#include <stdio.h>
#include <stdlib.h>
#include "../httpProcessing/commonHTTP.h"
#include "../net/networking.h"
#include "../utils.h"
#include "googleAccessToken.h"

//FIXME: these can be a VERY expensive functions to execute
//this whole page is a minefield, there's hdd access, send/recv'ing and printf/fget'ing

#define MAX_RETURN_CODE_LENGTH	 	1000
#define MAX_REFRESH_TOKEN_LENGTH 	1000
#define MAX_PASTED_CODE_LENGTH 		1000
#define MAX_ACCESS_TOKEN_LENGTH 	1000
#define CODE_REQUEST_POST_DATA "code=%s&"\
		"client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps.googleusercontent.com&"\
		"client_secret=BDkeIsUA394I_n-hwFp6syeO&"\
		"redirect_uri=urn:ietf:wg:oauth:2.0:oob&"\
		"grant_type=authorization_code"


void gat_newAccessTokenState(AccessTokenState_t *stateStruct){
	//malloc the intial stuff
	stateStruct->isAccessTokenLoaded 	= 0;
	stateStruct->accessTokenStr 		= malloc(MAX_ACCESS_TOKEN_LENGTH);
	stateStruct->accessTokenBufferLength = MAX_ACCESS_TOKEN_LENGTH;
	stateStruct->expireTime 			= 0;
	stateStruct->refreshTokenStr 		= malloc(MAX_REFRESH_TOKEN_LENGTH);
	stateStruct->refreshTokenBufferLength = MAX_REFRESH_TOKEN_LENGTH;
}

void gat_freeAccessTokenState(AccessTokenState_t *stateStruct){
	//TODO:
}

//TODO:
//check if we have enough room in the buffer and realloc if we don't
void setAccessToken(const char *accessToken, AccessTokenState_t *stateStruct){
	memcpy(stateStruct->accessTokenStr, accessToken, strlen(accessToken));
}

//TODO:
//check if we have enough room in the buffer and realloc if we don't
void setRefreshToken(const char *refreshToken, AccessTokenState_t *stateStruct){
	memcpy(stateStruct->refreshTokenStr, refreshToken, strlen(refreshToken));
}

//returns 0 if success, -1 if error where errorno is set, -2 otherwise
//if return code is non-0 accessTokenBuffer will be set to an empty string
//puts a '\0' terminated string in accessTokenBuffer
//it can return an invalid acces token
int loadFromFile(char *accessTokenBuffer, const int bufferLength){
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

void saveToFile(char *accessTokenBuffer, const int accessTokenLength){
	FILE *f = fopen("refresh.token", "w");
	fprintf(f, "%.*s\n", accessTokenLength, accessTokenBuffer);
	fclose(f);
}

//after this function we can be sure there'll be a refresh token in the refresh token file,
//after this function we can also be sure there'll be a refresh token in ram,
//hence you never have to loadFromFile after this
//we can also be sure that there'll be a access token in ram, although it may be expired
//returns 0 if success, -1 if error where errorno is set, -2 otherwise
int gat_init_googleAccessToken(AccessTokenState_t *stateStruct){
	int error;
	char refreshTokenBuffer[MAX_REFRESH_TOKEN_LENGTH];
	int shouldSaveToFile = 0;//boolean: true if the refresh token saved to file is invalid 

	gat_newAccessTokenState(stateStruct);

	//load it straight into the struct
	if(loadFromFile(refreshTokenBuffer, MAX_REFRESH_TOKEN_LENGTH) != 0){
		//if there's an error attempt to go on, pass on an invalid refresh token 
		refreshTokenBuffer[0] = '\0';
		shouldSaveToFile = 1;
	}

	setRefreshToken(refreshTokenBuffer, stateStruct);

	//try to use the refresh token to get an access token
	error = getAccessTokenWithRefreshToken(stateStruct);
	if(error != 0){
		shouldSaveToFile = 1;
		while(error != 0){
			//get a access token using the user's pasted code
			error = getNewTokensWithLink(stateStruct);
			if(error != 0){
				printf("Error: failed to get the access token, please retry\n");
			}
		}
	}

	if(shouldSaveToFile){
		saveToFile(stateStruct->accessTokenStr, strlen(stateStruct->accessTokenStr));
	}
	return 0;
}

//returns 0 if we got a valid access token from google, -1 if error with errorno, -2 otherwise
//refreshTokenStr must be null terminated
//FIXME: HARDCODED VALUES
int getAccessTokenWithRefreshToken(AccessTokenState_t *stateStruct){
	int size;
	char packetBuffer[1800];
	char url = "https://developers.google.com/oauth2/v3/token";
	char jsonReturnDataBuffer = malloc(200000);
	char refresh_token_data[] = "client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu."
								"apps.googleusercontent.com&"
								"client_secret=BDkeIsUA394I_n-hwFp6syeO&&"
								"refresh_token=%s&"
								"grant_type=refresh_token";
	headerInfo_t hInfo;
	Connection_t con;


	hInfo.contentLength = strlen(refresh_token_data) + strlen(stateStruct->accessTokenStr) - strlen("%s");

	size = utils_createHTTPHeaderFromUrl(url, packetBuffer, 1800, &hInfo, REQUEST_POST, NULL);
	//append the data
	sprintf(packetBuffer+size, refresh_token_data, stateStruct->accessTokenStr );

	utils_connectByUrl(url, &con);
	net_send();
	
	//download the data
	//receive the data until we're done
	//close the connection
	//parse the json and find what we're looking for,
	//set the stuff
	free(jsonReturnDataBuffer);
	return 0;
}

//sets both the refresh token and the access token
//returns 0 if we got a valid access token from google, -1 if error with errorno, -2 otherwise
//codeStr must be null terminated
int getAccessTokenWithPastedCode(const char *codeStr, AccessTokenState_t *stateStruct){
	return 0;
}

//sets both the refresh token and the access token
//returns 0 if we got a valid access token from google, -1 if error with errorno, -2 otherwise
int getNewTokensWithLink(AccessTokenState_t *stateStruct){
	char buffer[MAX_PASTED_CODE_LENGTH];

	printf("Visit this link and paste the code in...:\n"
		"https://accounts.google.com/o/oauth2/auth?access_type=offline"
		"&client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps."
		"googleusercontent.com&redirect_uri=urn%%3Aietf%%3Awg%%3Aoauth"
		"%%3A2.0%%3Aoob&response_type=code&scope=https%%3A%%2F%%2Fwww."
		"googleapis.com%%2Fauth%%2Fdrive\n");

	fgets(buffer, MAX_RETURN_CODE_LENGTH, stdin);
	buffer[ strlen(buffer) - 1 ] = '\0';//cut out the last char
	return getAccessTokenWithPastedCode( buffer, stateStruct );
}

void gat_getAccessToken(AccessTokenState_t *stateStruct){

	//check if the access token is valid
	if ( stateStruct->expireTime > (unsigned)time(NULL) ){
		;
	}else{
		getAccessTokenWithRefreshToken( stateStruct );
	}
}

