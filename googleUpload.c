#define header "POST /upload/drive/v2/files?uploadType=multipart HTTP/1.1\r\n"\
				"Host: www.googleapis.com\r\n"\
				"Authorization: Bearer your_auth_token\r\n"\
				"Content-Type: multipart/related; boundary=\"foo_bar_baz\"\r\n"\
				"Content-Length: "
#define packet_1 "\r\n\r\n--foo_bar_baz\n"\
					"Content-Type: application/json; charset=UTF-8\n"\
					"\n"\
					"{\n"\
					"  \"title\": \"My File\"\n"\
					"}\n"\
					"\n"\
					"--foo_bar_baz\n"\
					"Content-Type: multipart/encrypted\n\n"
#define packet_3 "\n--foo_bar_baz--\n"

//going to need to do some light state stuff on this side

//hm......this actually doesn't, the file has to be sent in one http "packet"
void getUploadPacket( char *inputdata, int inputDatalength, long fileSize, char *extraheaders, 
			char *contentlLengthStr, char *outputBuffer, int *outputBufferLength ){


	*outputBufferLength = inputDatalength + strlen(packet_3) + strlen(packet_1) 
							+ strlen(header) + strlen(contentlLengthStr);  
	int offset = 0;
	memcpy( outputBuffer+offset, header, strlen(header) );
	offset += strlen(header);
	memcpy( outputBuffer+offset, contentlLengthStr, strlen(contentlLengthStr) );
	offset += strlen(contentlLengthStr);
	memcpy( outputBuffer+offset, packet_1, strlen(packet_1) );
	offset += strlen(packet_1);
	memcpy( outputBuffer+offset, inputdata, inputDatalength );
	offset += inputDatalength;
	memcpy( outputBuffer+offset, packet_3, strlen(packet_3) );

}

void googleUpload_firstPacket(){

}

void googleUpload_middlePacket(){

}

void googleUpload_finalPacket(){

}

