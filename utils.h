//char* strstrn(char* const haystack, const char* needle, const int haystackSize);

//void getConnectionByUrl(const char *inputUrl, Connection_t *httpConnection, char **domain);

int utils_parseUrl(char *inputUrl, char **protocol, int *protocolLength,
		char **domain, int *domainLength, char **fileUrl, int *fileUrlLength);

int utils_parseUrl_proto(char *inputUrl, protocol_t *type, char **domain,
		int *domainLength, char **fileUrl, int *fileUrlLength);

int utils_recvNextHttpPacket(Connection_t *con, headerInfo_t *outputHInfo,
		char *outputBuffer, const int outputBufferMaxLength);

char *shitty_get_json_value(char* inputName, char* jsonData, int jsonDataSize);

char *getAccessTokenHeader(AccessTokenState_t *tokenState);

int utils_downloadHTTPFileSimple(char *outputBuffer, const int outputMaxLength,
		char *inputUrl, headerInfo_t *hInfo, char *extraHeaders);

int utils_createHTTPHeaderFromUrl(char *inputUrl, char *output,
		int maxOutputLen, headerInfo_t *hInfo,
		const httpRequestTypes_t requestType, char *extraHeaders);
