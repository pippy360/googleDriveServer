int googleUpload_init(Connection_t *con, AccessTokenState_t *accessTokenState,
		char *metadata, char *contentType);

int googleUpload_update(Connection_t *con, char *dataBuffer, int dataLength);

int googleUpload_end(Connection_t *con);


