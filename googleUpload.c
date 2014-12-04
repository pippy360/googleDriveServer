//todo create a header with all the max lenghts of the http stuff

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netDefines.h"
#include "googleUpload.h"

#define HEADER 	\
	"POST /upload/drive/v2/files?uploadType=multipart HTTP/1.1\r\n"\
	"Host: www.googleapis.com\r\n"\
	"Authorization: Bearer your_auth_token\r\n"\
	"Content-Type: multipart/related; boundary=\"foo_bar_baz\"\r\n"\
	"Content-Length: %lu\r\n\r\n"

#define PACKET_METADATA \
	"--foo_bar_baz\n"\
	"Content-Type: application/json; charset=UTF-8\n"\
	"\n"\
	"{\n"\
	"  \"title\": \"testUPload\"\n"\
	"}\n"\
	"\n"

#define PACKET_DATA	\
	"--foo_bar_baz\n"\
	"Content-Type: multipart/encrypted\n\n"

#define PACKET_END \
	"--foo_bar_baz--\n"



googleUploadStruct* getGoogleUploadStruct_struct(){
	googleUploadStruct *r = malloc( sizeof(googleUploadStruct) );
	r->stringOffset = 0;//used to keep track of how much of the header/metadata we've sent
	r->headerRemainingData = 0;
	r->fileRemainingData = 0;
	r->state = start;
	return r;
}

//todo: i don't like how hardcoded this is
//returns the amount of bytes not stored
int addHeader(long fileSize, char *outputData, int maxSize, int *addedLength, googleUploadStruct *stateStruct){
	int result;
	char headerBuffer[ HEADER_MAX_LENGTH ];

	sprintf( headerBuffer, HEADER, fileSize );
	
	int tempSize = strlen(headerBuffer) - stateStruct->stringOffset;
	if( tempSize > maxSize ){
		memcpy( outputData, headerBuffer + stateStruct->stringOffset, maxSize);
		stateStruct->stringOffset += maxSize;
		result = tempSize - maxSize;
	}else{
		memcpy( outputData, headerBuffer + stateStruct->stringOffset, tempSize);
		stateStruct->stringOffset = 0;
		result = 0;
	}

	return result;
}

//returns the amount of bytes not stored
int addMetaData(char *metaData, char *outputData, int maxSize, int *addedLength, googleUploadStruct *stateStruct){
	return 0;
}

//noChange is true if the outputData is going to be identical to the input, 
//in this case the function doesn't bother to copy the data to the outputData
int getNextUploadPacket(char *data, int dataLenght, unsigned long fileSize, googleUploadStruct *stateStruct,
						char *outputData, int outputDataLength, int *noChange){
	int result;
	char *dataPtr = data;
	int packetSize = 0;
	int addedLength;

	*noChange = 0;//default it to 0

	int maxSize = (MAX_PACKET_SIZE > outputDataLength)? outputDataLength : MAX_PACKET_SIZE;
	//FIXME, SHOULDN'T BE USING OUPUTDATA!!!!

	while( dataPtr != data + dataLenght && packetSize < maxSize){
		switch( stateStruct->state ){
			case start:
				stateStruct->fileRemainingData = fileSize;
				stateStruct->state = sendingHeader;
				break;
			case sendingHeader:
				if( addHeader( fileSize, outputData+packetSize, (maxSize-packetSize), 
								&addedLength, stateStruct ) == 0 ){
					
					stateStruct->state 		  = sendingMetaData;
					stateStruct->stringOffset = 0;
				}
				packetSize += addedLength;
				break;
			case sendingMetaData:
				//send as much of the header as we can
				//check if we can send the whole header
				if( addMetaData( "something", outputData+packetSize, (maxSize-packetSize), 
									&addedLength, stateStruct ) == 0 ){
					
					stateStruct->state 		  = sendingFileDataDelimiter;
					stateStruct->stringOffset = 0;
				}
				packetSize += addedLength;
				break;
			case sendingFileDataDelimiter:
				break;
			case sendingFileData:
				if ( data == dataPtr /* && the packet will be only ! data */)
				{
					result = 0;
					*noChange = 1;
				}else{
					/*append as much data as we can*/
				}
				break;
			case sendingEndDelimiter:

				break;
			case finished:
				// -1 can be taken as an error or used to tell when the file is finished
				result = -1;
				break;	
		}
	}

	return result;
}

int main(int argc, char const *argv[])
{
	//FIX ME: THERE SEEMS TO BE A BUG WHEN YOU RUN THIS
	googleUploadStruct *result = getGoogleUploadStruct_struct();
	char buffer[2000];
	int addedLength;
	result->stringOffset = 100;
	addHeader(12345, buffer, 100, &addedLength, result);
	printf("%s", buffer);
	return 0;
}