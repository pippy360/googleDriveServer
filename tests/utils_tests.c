#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../utils.h"
#include "../httpProcessing/commonHTTP.h"

void test_utils_parseUrl_proto(){

	char *domain, *fileUrl, status;
	int protocolLength, domainLength, fileUrlLength;
	protocol_t type;

	char url1[] = "https://example.s/something";
	status = utils_parseUrl_proto(url1, &type, &domain, &domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl_proto(url1) failed, not True\n");
	}else if( type != PROTO_HTTPS ){
		printf("ERROR: utils_parseUrl_proto(url1) failed, bad protocol type\n");
	}else if( domain != (url1+strlen("https://")) ){
		printf("ERROR: utils_parseUrl_proto(url1) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.s") ){
		printf("ERROR: utils_parseUrl_proto(url1) failed, bad domainLength\n");
	}else if( fileUrl != (url1+strlen("https://example.s")) ){
		printf("ERROR: utils_parseUrl_proto(url1) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != strlen("/something") ){
		printf("ERROR: utils_parseUrl_proto(url1) failed, bad fileUrlLength\n");
	}

	char url2[] = "http://example.s/something";
	status = utils_parseUrl_proto(url2, &type, &domain, &domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl_proto(url2) failed, not True\n");
	}else if( type != PROTO_HTTP ){
		printf("ERROR: utils_parseUrl_proto(url2) failed, bad protocol type\n");
	}else if( domain != (url2+strlen("http://")) ){
		printf("ERROR: utils_parseUrl_proto(url2) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.s") ){
		printf("ERROR: utils_parseUrl_proto(url2) failed, bad domainLength\n");
	}else if( fileUrl != (url2+strlen("http://example.s")) ){
		printf("ERROR: utils_parseUrl_proto(url2) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != strlen("/something") ){
		printf("ERROR: utils_parseUrl_proto(url2) failed, bad fileUrlLength\n");
	}

}

void test_utils_parseUrl(){

	//invalid
	char url1[] = "http://something that's not a url";
	char url2[] = "example.com";//also not a url, url's have to have "*://"
	//valid
	char url3[] = "https://example.s";
	char url4[] = "https://example.";

	char *protocol, *domain, *fileUrl, status;
	int protocolLength, domainLength, fileUrlLength;

	status = utils_parseUrl(url1, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 0 ){
		printf("ERROR: utils_parseUrl(url1) failed\n");
	}

	status = utils_parseUrl(url2, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 0 ){
		printf("ERROR: utils_parseUrl(url2) failed\n");
	}

	//valid
	status = utils_parseUrl(url3, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url3) failed, not True\n");
	}else if( protocol != url3 ){
		printf("ERROR: utils_parseUrl(url3) failed, bad protocol pointer\n");
	}else if( protocolLength != 5 ){
		printf("ERROR: utils_parseUrl(url3) failed, bad protocolLength\n");
	}else if( domain != (url3+strlen("https://")) ){
		printf("ERROR: utils_parseUrl(url3) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.s") ){
		printf("ERROR: utils_parseUrl(url3) failed, bad domainLength\n");
	}else if( fileUrl != (url3+strlen(url3)) ){
		printf("ERROR: utils_parseUrl(url3) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != 0 ){
		printf("ERROR: utils_parseUrl(url3) failed, bad fileUrlLength\n");
	}

	status = utils_parseUrl(url4, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url4) failed, not True\n");
	}else if( protocol != url4 ){
		printf("ERROR: utils_parseUrl(url4) failed, bad protocol pointer\n");
	}else if( protocolLength != 5 ){
		printf("ERROR: utils_parseUrl(url4) failed, bad protocolLength\n");
	}else if( domain != (url4+strlen("https://")) ){
		printf("ERROR: utils_parseUrl(url4) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.") ){
		printf("ERROR: utils_parseUrl(url4) failed, bad domainLength\n");
	}else if( fileUrl != (url4+strlen(url4)) ){
		printf("ERROR: utils_parseUrl(url4) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != 0 ){
		printf("ERROR: utils_parseUrl(url4) failed, bad fileUrlLength\n");
	}

	char url5[] = "https://.";
	status = utils_parseUrl(url5, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url5) failed, not True\n");
	}else if( protocol != url5 ){
		printf("ERROR: utils_parseUrl(url5) failed, bad protocol pointer\n");
	}else if( protocolLength != 5 ){
		printf("ERROR: utils_parseUrl(url5) failed, bad protocolLength\n");
	}else if( domain != (url5+strlen("https://")) ){
		printf("ERROR: utils_parseUrl(url5) failed, bad domain pointer\n");
	}else if( domainLength != strlen(".") ){
		printf("ERROR: utils_parseUrl(url5) failed, bad domainLength\n");
	}else if( fileUrl != (url5+strlen(url5)) ){
		printf("ERROR: utils_parseUrl(url5) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != 0 ){
		printf("ERROR: utils_parseUrl(url5) failed, bad fileUrlLength\n");
	}
	
	char url6[] = "https://.com";
	status = utils_parseUrl(url6, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url6) failed, not True\n");
	}else if( protocol != url6 ){
		printf("ERROR: utils_parseUrl(url6) failed, bad protocol pointer\n");
	}else if( protocolLength != strlen("https") ){
		printf("ERROR: utils_parseUrl(url6) failed, bad protocolLength\n");
	}else if( domain != (url6+strlen("https://")) ){
		printf("ERROR: utils_parseUrl(url6) failed, bad domain pointer\n");
	}else if( domainLength != strlen(".com") ){
		printf("ERROR: utils_parseUrl(url6) failed, bad domainLength\n");
	}else if( fileUrl != (url6+strlen(url6)) ){
		printf("ERROR: utils_parseUrl(url6) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != 0 ){
		printf("ERROR: utils_parseUrl(url6) failed, bad fileUrlLength\n");
	}

	char url7[] = "ftp://example.com";
	status = utils_parseUrl(url7, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url7) failed, not True\n");
	}else if( protocol != url7 ){
		printf("ERROR: utils_parseUrl(url7) failed, bad protocol pointer\n");
	}else if( protocolLength != strlen("ftp") ){
		printf("ERROR: utils_parseUrl(url7) failed, bad protocolLength\n");
	}else if( domain != (url7+strlen("ftp://")) ){
		printf("ERROR: utils_parseUrl(url7) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.com") ){
		printf("ERROR: utils_parseUrl(url7) failed, bad domainLength\n");
	}else if( fileUrl != (url7+strlen(url7)) ){
		printf("ERROR: utils_parseUrl(url7) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != 0 ){
		printf("ERROR: utils_parseUrl(url7) failed, bad fileUrlLength\n");
	}

	char url8[] = "http://example.com/ofasdfasdf/asdfasfd/";
	status = utils_parseUrl(url8, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url8) failed, not True\n");
	}else if( protocol != url8 ){
		printf("ERROR: utils_parseUrl(url8) failed, bad protocol pointer\n");
	}else if( protocolLength != strlen("http") ){
		printf("ERROR: utils_parseUrl(url8) failed, bad protocolLength\n");
	}else if( domain != (url8+strlen("http://")) ){
		printf("ERROR: utils_parseUrl(url8) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.com") ){
		printf("ERROR: utils_parseUrl(url8) failed, bad domainLength\n");
	}else if( fileUrl != (url8+strlen("http://example.com")) ){
		printf("ERROR: utils_parseUrl(url8) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != strlen("/ofasdfasdf/asdfasfd/") ){
		printf("ERROR: utils_parseUrl(url8) failed, bad fileUrlLength\n");
	}

	char url9[] = "https://example.com/";
	status = utils_parseUrl(url9, &protocol, &protocolLength, &domain, 
						&domainLength, &fileUrl, &fileUrlLength);
	if( status != 1 ){
		printf("ERROR: utils_parseUrl(url9) failed, not True\n");
	}else if( protocol != url9 ){
		printf("ERROR: utils_parseUrl(url9) failed, bad protocol pointer\n");
	}else if( protocolLength != strlen("https") ){
		printf("ERROR: utils_parseUrl(url9) failed, bad protocolLength\n");
	}else if( domain != (url9+strlen("https://")) ){
		printf("ERROR: utils_parseUrl(url9) failed, bad domain pointer\n");
	}else if( domainLength != strlen("example.com") ){
		printf("ERROR: utils_parseUrl(url9) failed, bad domainLength\n");
	}else if( fileUrl != (url9+strlen("https://example.com")) ){
		printf("ERROR: utils_parseUrl(url9) failed, bad fileUrl pointer\n");
	}else if( fileUrlLength != strlen("/") ){
		printf("ERROR: utils_parseUrl(url9) failed, bad fileUrlLength\n");
	}
}


int main(){


	test_utils_parseUrl();
	test_utils_parseUrl_proto();

	return 0;
}