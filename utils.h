//char* strstrn(char* const haystack, const char* needle, const int haystackSize);

//void getConnectionByUrl(const char *inputUrl, Connection_t *httpConnection, char **domain);

int utils_parseUrl(char *inputUrl, char **protocol, int *protocolLength, 
                    char **domain, int *domainLength, char **fileUrl, 
                    int *fileUrlLength );