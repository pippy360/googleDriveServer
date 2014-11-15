//getters and setters for json, headers, url parsing
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

#define MAX_STRING_SIZE 2000
#define FORMAT "\"%s\" : "

//todo:
void isFormatChar(){

}

int isJsonChar( char inputChar ){
	if ( ('a' <= inputChar && inputChar <= 'z') || ('A' <= inputChar && inputChar <= 'Z') 
		|| ( '0' <= inputChar && inputChar <= '9' ) || ( '"' == inputChar ) ){
		return 1;
	}else{
		return 0;
	}
}

//TODO: BUG TEST THIS
//TODO: clean up
char* get_json_value(char* inputName, char* jsonData, int jsonDataSize){
	//find the area, get the value
	char needle[MAX_STRING_SIZE];
	sprintf( needle, FORMAT, inputName);
	char* ptr;
	if((ptr = strstrn(jsonData, needle, jsonDataSize)) == NULL)
		return NULL;

	ptr += strlen( inputName ) + strlen( FORMAT ) - strlen("%s");

	//now find the value

	//get the starting char
	for( ; !isJsonChar( *ptr ); ptr++ )
		;

	int isString = 0;
	if (*ptr == '\"'){
		isString = 1;
		ptr++;
	}

	char *endPtr;
	if ( isString )
	{
		for( endPtr = ptr+1; *endPtr != '\"'; endPtr++ )
			;
	}else{
		for( endPtr = ptr; *endPtr != ','; endPtr++ )
			;
	}

	int outputLength = endPtr - ptr;
	char* output = malloc( outputLength + 1 );
	memcpy(output, ptr, outputLength );
	output[ outputLength ] = '\0';
	return output;
}

//TODO: enum for http/https/ftp
//this will break a lot, "example.com" breaks it
int parseUrl(char* inputUrl, int* type, char** domain, char** fileUrl){
    //break at '://' and check if it matches http/https
    //strstrn
    char *ptr = strstrn(inputUrl, "://", strlen(inputUrl));
    //get the type by cmping

    int protoSize = ptr - inputUrl;

    if( strncmp( inputUrl, "https", protoSize) == 0 ){
    	*type = 1;
    }else{
    	*type = -1;
    }

    //get the url (until we hit "/" )
    char* newPtr = strstrn(ptr+3, "/", strlen(inputUrl) - protoSize - 3);
    *domain = malloc( (newPtr - (ptr+3)) + 1 );
    (*domain)[ (newPtr - (ptr+3)) ] = '\0';
    memcpy( *domain, ptr+3, (newPtr - (ptr+3)) );

    *fileUrl = malloc( (strlen(inputUrl) - (newPtr - inputUrl)) + 1 );
    (*fileUrl)[ (strlen(inputUrl) - (newPtr - inputUrl)) + 1 ] = '\0';
    memcpy( *fileUrl, newPtr, (strlen(inputUrl) - (newPtr - inputUrl)) );
}
