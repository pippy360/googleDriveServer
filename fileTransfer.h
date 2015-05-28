typedef struct {
	long encryptedDataDownloaded; //the amount of the encrypted file downloaded (doesn't include things like http headers)
	long clientRangeStart;
	long clientRangeEnd;
	long encryptedRangeStart;
	long encryptedRangeEnd;
	CryptoState_t cryptoState;
} CryptoFileDownloadState_t;

int startFileDownload(char *inputUrl, char isRangedRequest, char isPartialRange,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		char *extraHeaders);

int updateFileDownload(Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, char *extraHeaders);

int finishFileDownload();
