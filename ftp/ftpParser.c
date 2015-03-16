#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ftpCommon.h"

void ftp_newParserState(ftpParserState_t *state, char *paramBuffer, int paramBufferLength){
	state->type = REQUEST_ERROR;
	state->paramBuffer = paramBuffer;
	state->paramBuffer[0] = '\0';
	state->paramBufferLength = paramBufferLength;
	//FIXME: SET THE ERROR STUFF
}

int getFtpCommand(char *packet, int packetLength, char *buffer, int bufferLength, char *endChar, char **endPos){

	int i;
	for (i = 0; i+1<bufferLength && i+1<packetLength; ++i)
	{
		if (	packet[i] == '\r' && packet[i+1] == '\n'){
			if(endPos != NULL)
				*endPos = packet + i + 2;
			
			buffer[i] = '\0';
			*endChar = '\n';
			return 0;
		}else if(packet[i] == ' '){
			if(endPos != NULL)
				*endPos = packet + i + 1;
		
			buffer[i] = '\0';
			*endChar = ' ';
			return 0;
		}

		buffer[i] = packet[i];
	}
	return -1;
}

//0 if success, non-0 otherwise
//ptr points to the start of the param
int getFtpParam(char *packet, int packetLength, char *ptr, ftpParserState_t *parserState){
	//reset the values
	if (*ptr == '\r'){
		parserState->paramBuffer[0] = '\0';
		return -1;
	}

	int remainingLength = packetLength - (ptr - packet);
	int i;
	for (i = 0; i+1 < remainingLength && i+1 < parserState->paramBufferLength; ++i){
		if(ptr[i] == '\r' && ptr[i+1] == '\n'){
			parserState->paramBuffer[i] = '\0';
			return 0;
		}else if (!isprint(ptr[i])){//check if valid char
			//FIXME: SET THE ERROR STUFF
			return -1;
		}
		parserState->paramBuffer[i] = ptr[i];
	}

	parserState->paramBuffer[i] = '\0';//make it easier to debug
	return -1;
}

//0 if success, non-0 otherwise
//parses the request and the parameter
int ftp_parsePacket(char *packet, int packetLength, ftpParserState_t *parserState, ftpClientState_t *clientState){

	char buffer[MAX_PACKET_LENGTH], endChar, *endPos;

	//0 if success, non-0 otherwise
	if(getFtpCommand(packet, packetLength, buffer, MAX_PACKET_LENGTH, &endChar, &endPos) != 0){
		printf("we got here !!\n");
		parserState->type = REQUEST_ERROR;
		return -1;
	}
	
	if(endChar == ' '){
		if (getFtpParam(packet, packetLength, endPos, parserState) != 0 
				&& strlen(parserState->paramBuffer) != 0){
			//FIXME: MAKE the ERROR STUFF
			return -1;
		}
	}else{
		parserState->paramBuffer[0] = '\0';
	}

	//uppercase the  
	char *p = buffer;
	while((*p = toupper(*p)))
		p++;

	if( 	  strcmp(buffer, "ABOR") == 0 ){
		parserState->type = REQUEST_ABOR;
	}else if( strcmp(buffer, "ACCT") == 0 ){
		parserState->type = REQUEST_ACCT;
	}else if( strcmp(buffer, "ALLO") == 0 ){
		parserState->type = REQUEST_ALLO;
	}else if( strcmp(buffer, "APPE") == 0 ){
		parserState->type = REQUEST_APPE;
	}else if( strcmp(buffer, "CWD") == 0 ){
		parserState->type = REQUEST_CWD;
	}else if( strcmp(buffer, "CDUP") == 0 ){
		parserState->type = REQUEST_CDUP;
	}else if( strcmp(buffer, "DELE") == 0 ){
		parserState->type = REQUEST_DELE;
	}else if( strcmp(buffer, "HELP") == 0 ){
		parserState->type = REQUEST_HELP;
	}else if( strcmp(buffer, "LIST") == 0 ){
		parserState->type = REQUEST_LIST;
	}else if( strcmp(buffer, "MKD") == 0 ){
		parserState->type = REQUEST_MKD;
	}else if( strcmp(buffer, "MDTM") == 0 ){
		parserState->type = REQUEST_MDTM;
	}else if( strcmp(buffer, "MODE") == 0 ){
		parserState->type = REQUEST_MODE;
	}else if( strcmp(buffer, "NLST") == 0 ){
		parserState->type = REQUEST_NLST;
	}else if( strcmp(buffer, "NOOP") == 0 ){
		parserState->type = REQUEST_NOOP;
	}else if( strcmp(buffer, "PASS") == 0 ){
		parserState->type = REQUEST_PASS;
	}else if( strcmp(buffer, "PASV") == 0 ){
		parserState->type = REQUEST_PASV;
	}else if( strcmp(buffer, "PORT") == 0 ){
		parserState->type = REQUEST_PORT;
	}else if( strcmp(buffer, "PWD") == 0 ){
		parserState->type = REQUEST_PWD;
	}else if( strcmp(buffer, "QUIT") == 0 ){
		parserState->type = REQUEST_QUIT;
	}else if( strcmp(buffer, "RETR") == 0 ){
		parserState->type = REQUEST_RETR;
	}else if( strcmp(buffer, "RMD") == 0 ){
		parserState->type = REQUEST_RMD;
	}else if( strcmp(buffer, "RNFR") == 0 ){
		parserState->type = REQUEST_RNFR;
	}else if( strcmp(buffer, "RNTO") == 0 ){
		parserState->type = REQUEST_RNTO;
	}else if( strcmp(buffer, "SITE") == 0 ){
		parserState->type = REQUEST_SITE;
	}else if( strcmp(buffer, "UMASK") == 0 ){
		parserState->type = REQUEST_UMASK;
	}else if( strcmp(buffer, "IDLE") == 0 ){
		parserState->type = REQUEST_IDLE;
	}else if( strcmp(buffer, "CHMOD") == 0 ){
		parserState->type = REQUEST_CHMOD;
	}else if( strcmp(buffer, "HELP") == 0 ){
		parserState->type = REQUEST_HELP;
	}else if( strcmp(buffer, "SIZE") == 0 ){
		parserState->type = REQUEST_SIZE;
	}else if( strcmp(buffer, "STAT") == 0 ){
		parserState->type = REQUEST_STAT;
	}else if( strcmp(buffer, "STOR") == 0 ){
		parserState->type = REQUEST_STOR;
	}else if( strcmp(buffer, "STOU") == 0 ){
		parserState->type = REQUEST_STOU;
	}else if( strcmp(buffer, "STRU") == 0 ){
		parserState->type = REQUEST_STRU;
	}else if( strcmp(buffer, "SYST") == 0 ){
		parserState->type = REQUEST_SYST;
	}else if( strcmp(buffer, "TYPE") == 0 ){
		parserState->type = REQUEST_TYPE;
	}else if( strcmp(buffer, "USER") == 0 ){
		parserState->type = REQUEST_USER;
	}else if( strcmp(buffer, "XCUP") == 0 ){
		parserState->type = REQUEST_XCUP;
	}else if( strcmp(buffer, "XCWD") == 0 ){
		parserState->type = REQUEST_XCWD;
	}else if( strcmp(buffer, "XMKD") == 0 ){
		parserState->type = REQUEST_XMKD;
	}else if( strcmp(buffer, "XPWD") == 0 ){
		parserState->type = REQUEST_XPWD;
	}else if( strcmp(buffer, "XRMD") == 0 ){
		parserState->type = REQUEST_XRMD;
	}else{
		parserState->type = REQUEST_ERROR;
	}

	return 0;
}

