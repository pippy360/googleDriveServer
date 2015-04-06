typedef struct {
	long startBytePos;
	long endBytePos;
	char key[32];
	char iv[16];
	EVP_CIPHER_CTX *ctx;
} CryptoState_t;
