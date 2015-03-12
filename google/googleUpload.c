//todo create a header with all the max lenghts of the http stuff

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netDefines.h"
#include "googleUpload.h"

#define HEADER 	\
	"POST /upload/drive/v2/files?uploadType=multipart HTTP/1.1\r\n"\
	"Host: www.googleapis.com\r\n"\
	"%s"\
	"Content-Type: multipart/related; boundary=\"foo_bar_baz\"\r\n"\
	"Content-Length: %lu\r\n\r\n"

#define PACKET_METADATA \
	"--foo_bar_baz\n"\
	"Content-Type: application/json; charset=UTF-8\n"\
	"\n"\
	"%s"\
	"\n"

#define PACKET_DATA	\
	"--foo_bar_baz\n"\
	"Content-Type: text/plain\n\n"

#define PACKET_END \
	"\n--foo_bar_baz--\n"



googleUploadStruct* getGoogleUploadStruct_struct(){
	googleUploadStruct *r = malloc( sizeof(googleUploadStruct) );
	r->stringOffset = 0;//used to keep track of how much of the header/metadata we've sent
	r->headerRemainingData = 0;
	r->fileRemainingData = 0;
	r->state = start;
	return r;
}

//TODO: implement nochange
//EXPLAIN HOW THE STRING OFFSET WORKS
//returns the amount of bytes not copied to the packet
int copy_sting_to_packet(char *inputString, char *outputBuffer, int outputBufferSize, int *addedLength, 
						googleUploadStruct *stateStruct){
	int result;
		
	int tempSize = strlen(inputString) - stateStruct->stringOffset;
	if( tempSize > outputBufferSize ){
		memcpy( outputBuffer, inputString + stateStruct->stringOffset, outputBufferSize);
		stateStruct->stringOffset += outputBufferSize;
		*addedLength = outputBufferSize;
		result = tempSize - outputBufferSize;
	}else{
		memcpy( outputBuffer, inputString + stateStruct->stringOffset, tempSize);
		stateStruct->stringOffset = 0;
		*addedLength = tempSize;
		result = 0;
	}

	return result;
}

//TODO: SO MUCH INPUT, clean up the parameters
//returns the size of the packet
//noChange is true if the outputBuffer is going to be identical to the input, 
//in this case the function doesn't bother to copy the data to the outputBuffer
int getNextUploadPacket(char *data, int dataLength, int *dataSent, unsigned long filesize, char *metadata, 
						char *addedHeaders, googleUploadStruct *stateStruct, char *outputBuffer, int outputBufferSize, 
						int *noChange){
	
	*dataSent = 0;
	int result = 0;
	int packetSize = 0;
	int addedLength;
	char stringBuffer[ HEADER_MAX_LENGTH ];//todo: HEADER_MAX_LENGTH isn't the constant we should be using
	char *dataPtr;
	*noChange = 0;//default it to 0

	int maxSize = (MAX_PACKET_SIZE > outputBufferSize)? outputBufferSize : MAX_PACKET_SIZE;
	//FIXME, SHOULDN'T BE USING OUPUTDATA!!!!

	long contentLength = strlen(PACKET_METADATA) - strlen("%s") + strlen(metadata) + strlen(PACKET_DATA) + strlen(PACKET_END) + filesize;  

	while( stateStruct->state != finished && packetSize < maxSize && *dataSent == 0 ){
		switch( stateStruct->state ){
			case start:
				stateStruct->fileRemainingData = filesize;
				stateStruct->state = sendingHeader;
				break;
			case sendingHeader:
				sprintf( stringBuffer, HEADER, addedHeaders, contentLength );//FIXME: it's not the filesize !!
				if( copy_sting_to_packet( stringBuffer, outputBuffer+packetSize, (maxSize-packetSize), 
									&addedLength, stateStruct ) == 0 ){
					
					stateStruct->state 		  = sendingMetaData;
					stateStruct->stringOffset = 0;
				}
				packetSize += addedLength;
				break;
			case sendingMetaData:
				sprintf( stringBuffer, PACKET_METADATA, metadata );
				if( copy_sting_to_packet( stringBuffer, outputBuffer+packetSize, (maxSize-packetSize),  
									&addedLength, stateStruct ) == 0 ){
					
					stateStruct->state 		  = sendingFileDataDelimiter;
					stateStruct->stringOffset = 0;
				}
				packetSize += addedLength;
				break;
			case sendingFileDataDelimiter:
				if( copy_sting_to_packet( PACKET_DATA, outputBuffer+packetSize, (maxSize-packetSize), 
									&addedLength, stateStruct ) == 0 ){

					stateStruct->state 		  = sendingFileData;
					stateStruct->stringOffset = 0;
				}
				packetSize += addedLength;
				break;
			case sendingFileData:
				//FIXME: THIS DOESN'T WORK IF YOU HAVE TO REPEAT THE DATA
				if ( dataLength + packetSize > maxSize ){
					//we can't fit all the data in, put as much as we can in
					memcpy( outputBuffer+packetSize, data, maxSize-packetSize);
					*dataSent = maxSize-packetSize;
					packetSize += maxSize-packetSize;
					stateStruct->fileRemainingData -= maxSize-packetSize;
				}else{
					//we can fit all the remaining data into the packet
					memcpy( outputBuffer+packetSize, data, dataLength);
					*dataSent = dataLength;
					packetSize += dataLength;
					stateStruct->fileRemainingData -= dataLength;
				}
				if (stateStruct->fileRemainingData == 0)
				{
					stateStruct->state = sendingEndDelimiter;
				}else{
				}
				break;
			case sendingEndDelimiter:
				if( copy_sting_to_packet( PACKET_END, outputBuffer+packetSize, (maxSize-packetSize), 
									&addedLength, stateStruct ) == 0 ){

					stateStruct->state 		  = finished;
					stateStruct->stringOffset = 0;
					//todo: test to make sure that this null terminator will always fit
					outputBuffer[packetSize + addedLength] = '\0';
				}
				packetSize += addedLength;
				break;
		}
	}
	return packetSize;
}

/*
int main(int argc, char const *argv[])
{
	googleUploadStruct *result = getGoogleUploadStruct_struct();
	char buffer[2000];
	int addedLength;
	int noChange;
	int packetSize;
	while( result->state != finished ){	
		packetSize = getNextUploadPacket("kicked out\n", strlen("kicked out\n"), (long)strlen("kicked out\n"), 
			"{\n  \"title\": \"My File\"\n}\n", "addHead\r\n", result, buffer, 2000, &noChange);
		printf("/--%.*s--/\n", packetSize, buffer);
	}

	return 0;
}
*/
