#ifndef CRYPTO_H
#define CRYPTO_H

#define AES_BLOCK_SIZE 16//TODO: replace with the openssl block size define

typedef struct {
	long startBytePos;
	long endBytePos;
	unsigned char key[32];
	unsigned char iv[16];
	unsigned char salt[8];
	EVP_CIPHER_CTX *ctx;
} CryptoState_t;

int startEncryption(CryptoState_t *state, const char *password);

int updateEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize);

int finishEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize);

int startDecryption(CryptoState_t *state, const char *password, const char *iv);

int updateDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize);

int finishDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize);

#endif /* CRYPTO_H */