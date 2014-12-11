//this file is spaghetti

#include <stdio.h>
#include <stdlib.h>
#include "net/networking.h"
#include "parser.h"

#define MAX_RETURN_CODE_LENGTH 1600
#define CLIENT_ID 400
#define CLIENT_SECRET 400
#define MAX_HEADER_SIZE 1000
#define MAXDATASIZE 2000
#define HEADER_FORMAT "POST /o/oauth2/token HTTP/1.1\r\nHost: accounts.google.com\r\n"\
					"Content-Type: application/x-www-form-urlencoded\r\n"\
					"Content-length: %d\r\n\r\n%s"
#define CODE_REQUEST_POST_DATA "code=%s&"\
		"client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps.googleusercontent.com&"\
		"client_secret=BDkeIsUA394I_n-hwFp6syeO&"\
		"redirect_uri=urn:ietf:wg:oauth:2.0:oob&"\
		"grant_type=authorization_code"
#define REFRESH_TOKEN_DATA "client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps.googleusercontent.com&"\
		"client_secret=BDkeIsUA394I_n-hwFp6syeO&&"\
		"refresh_token=%s&"\
		"grant_type=refresh_token"


char* 		urlTemplate 			= "https://accounts.google.com/o/oauth2/auth?access_type=offline&client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps.googleusercontent.com&redirect_uri=urn%3Aietf%3Awg%3Aoauth%3A2.0%3Aoob&response_type=code&scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fdrive";
char* 		currentAccessToken 		= NULL;
unsigned   	ATExpireTime 			= 0;
char* 		currentRefreshToken 	= NULL;

void loadFromFile(){
	char * line = NULL;
	size_t zero = 0;
	FILE *f = fopen("refresh.token", "r");
	if (f == NULL)
		exit(EXIT_FAILURE);
	
	rewind(f);
	int length = getline(&line, &zero, f);
	currentRefreshToken = line;
	//todo: handle windows line ending!!!
	currentRefreshToken[length-1] = '\0';

	fclose(f);
}

void saveToFile(){
	FILE *f = fopen("refresh.token", "w");
	fprintf(f, "%s\n", currentRefreshToken);
	fclose(f);
}

void google_init(){
	loadFromFile();
}

char* getAccessTokenHTTPHeaders( char* code ){
	char* output = malloc( MAX_HEADER_SIZE );
	int contentLength = strlen( CODE_REQUEST_POST_DATA ) + 
						strlen( code ) - 2;

	sprintf( output, "POST /o/oauth2/token HTTP/1.1\r\nHost: accounts.google.com\r\n"
					"Content-Type: application/x-www-form-urlencoded\r\n"
					"Content-length: %d\r\n\r\n" CODE_REQUEST_POST_DATA, contentLength, code );

	return output;
}

char* getRefreshTokenHTTPHeaders( char* refresh_token ){
	char* output = malloc( MAX_HEADER_SIZE );
	int contentLength = strlen( REFRESH_TOKEN_DATA ) + 
						strlen( refresh_token ) - 2;

	sprintf( output, "POST /o/oauth2/token HTTP/1.1\r\nHost: accounts.google.com\r\n"
					"Content-Type: application/x-www-form-urlencoded\r\n"
					"Content-length: %d\r\n\r\n" REFRESH_TOKEN_DATA, contentLength, refresh_token );

	return output;
}

//TODO:
//a badly named function, 
//it sends a request to google with the supplied data and returns the 
//access token/refresh token/time fromt the json
//TODO: CLEAN UP
void send_to_google(char* data, char** access_token, char** refresh_token, 
				int* expire_time, char** error, char** error_description)
{
	//create the packet
	char packet[ MAXDATASIZE ];
	sprintf( packet, HEADER_FORMAT, (int)strlen(data), data);

	//then send the packet
	connection* c = sslConnect ("accounts.google.com", "443");
	SSL_write (c->sslHandle, packet, strlen (packet));
	
	//and parse the reply
	char* buffer = malloc( MAXDATASIZE );
	int received = SSL_read (c->sslHandle, buffer, MAXDATASIZE-1);
	buffer[received] = '\0';

	*access_token 		= get_json_value("access_token", buffer, received);
	*refresh_token 		= get_json_value("refresh_token", buffer, received);
	*error				= get_json_value("error", buffer, received);
	*error_description 	= get_json_value("error_description", buffer, received);
	char* timeStr		= get_json_value("expires_in", buffer, received);

	if (timeStr != NULL)
		*expire_time = time(NULL) + (int) strtol(timeStr, NULL, 10);

	sslDisconnect (c);
}

//also sets the global vars
char *getAccessTokenWithRefreshToken(char *input_refresh_token){
	char data[MAX_RETURN_CODE_LENGTH + 1];
	char *refresh_token, *access_token, *error, *error_description;
	int expires_in;

	sprintf( data, REFRESH_TOKEN_DATA, input_refresh_token );
	send_to_google(data, &access_token, &refresh_token, &expires_in, &error, &error_description);

	if (access_token == NULL)
	{
		printf("ERROR %s\n", error);
		if( error_description != NULL )
			printf("%s\n", error_description);

		return NULL;
	}
	currentAccessToken 		= access_token;
	ATExpireTime 			= expires_in;

	return access_token;
}

char* getAccessTokenWithPastedCode(char *code){
	char data[MAX_RETURN_CODE_LENGTH + 1];
	char *refresh_token, *access_token, *error, *error_description;
	int expires_in;

	sprintf( data, CODE_REQUEST_POST_DATA, code );
	send_to_google(data, &access_token, &refresh_token, &expires_in, &error, &error_description);

	currentAccessToken 		= access_token;
	ATExpireTime 			= expires_in;
	currentRefreshToken 	= refresh_token;

	return access_token;	
}

char* getNewAccessTokenWithLink(){
	printf("Visit this link and paste the code in...:\n");
	printf("https://accounts.google.com/o/oauth2/auth?access_type=offline&client_id=83270242690-k8gfaaaj5gjahc7ncvri5m3pu2lp9nsu.apps.googleusercontent.com&redirect_uri=urn%%3Aietf%%3Awg%%3Aoauth%%3A2.0%%3Aoob&response_type=code&scope=https%%3A%%2F%%2Fwww.googleapis.com%%2Fauth%%2Fdrive\n");
	char buffer[MAX_RETURN_CODE_LENGTH + 1];
	fgets(buffer, MAX_RETURN_CODE_LENGTH, stdin);
	buffer[ strlen(buffer) - 1 ] = '\0';
	return getAccessTokenWithPastedCode( buffer );
}

//todo: explain
char* getAccessToken(){

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


//THERE WAS AN ERROR HERE, WHERE YOU FREE'D A GLOBAL VAR, MAKE SURE THAT DOESN'T HAPPEN AGAIN
char* getAccessTokenHeader(){

    char *accessToken = getAccessToken();
    char headerStub[]  = "Authorization: Bearer ";

    char* tokenHeader = malloc( strlen(headerStub) + strlen(accessToken) + 1 + 2 );
    sprintf( tokenHeader, "%s%s%s", headerStub, accessToken, "\r\n");

    return tokenHeader;
}