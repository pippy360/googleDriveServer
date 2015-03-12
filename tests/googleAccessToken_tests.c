#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../google/googleAccessToken.h"
#include "../google/googleUpload.h"



void test_utils_parseUrl_proto(){
	int accessTokenLength;
	char buffer[10000];
	AccessTokenState_t stateStruct;

	gat_init_googleAccessToken(&stateStruct);

	gat_getAccessToken(&stateStruct, &accessTokenLength);

	gat_freeAccessTokenState(&stateStruct);
}

int main(int argc, char const *argv[])
{
	test_utils_parseUrl_proto();
	return 0;
}