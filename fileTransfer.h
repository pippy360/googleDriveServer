typedef struct {
	//the amount of the encrypted file downloaded
	//(doesn't include things like http headers,
	//but does include any meta data at the start of the encrypted file like the IV)
	long encryptedDataDownloaded;
	long amountOfFileDecrypted;
	long clientRangeStart;
	long clientRangeEnd;
	long encryptedRangeStart;
	long encryptedRangeEnd;
	CryptoState_t cryptoState;
	char isEndRangeSet;
	char isDecryptionComplete;
} CryptoFileDownloadState_t;

int startFileDownload(char *inputUrl, char isRangedRequest, char isPartialRange,
		long startRange, long endRange, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		char *extraHeaders);

int updateFileDownload(Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, char *extraHeaders);

int finishFileDownload();
