int startFileDownload(char *inputUrl, Connection_t *con,
		headerInfo_t *outputHInfo, parserState_t *outputParserState,
		char *extraHeaders);

int updateFileDownload(Connection_t *con, headerInfo_t *outputHInfo,
		parserState_t *outputParserState, char *outputBuffer,
		int outputBufferMaxLength, int *outputBufferLength, char *extraHeaders);

int finishFileDownload();
