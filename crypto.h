#define AES_BLOCK_SIZE 16//TODO: replace with the openssl block size define

typedef struct {
	long startBytePos;
	long endBytePos;
	char key[32];
	char iv[16];
	char salt[8];
	EVP_CIPHER_CTX *ctx;
} CryptoState_t;
