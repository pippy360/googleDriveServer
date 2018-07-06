#include <stdio.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include "crypto.h"

#define IV_LENGTH 16 //FIXME: double check

void init_opensll() {
	/* Initialise the library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);
}

//password MUST BE '\0' terminated
//saltBuffer MUST BE 8 bytes
//the input buffer doesn't have to have any data in it, it can be null
//returns 0 if success, -1 otherwise

//must be at the start of file
int startEncryption(CryptoState_t *state, const char *password) {

	/* get the key, iv and salt*/
	char salt[] = "12345678"; //randomly gen a salt, TODO:
	memcpy(state->salt, salt, 8);
	EVP_BytesToKey(EVP_aes_128_cbc(), EVP_sha1(), salt, password,
			strlen(password), 4, state->key, state->iv);

	if (!(state->ctx = EVP_CIPHER_CTX_new()))
		return -1;

	if (EVP_EncryptInit_ex(state->ctx, EVP_aes_128_cbc(), NULL, state->key,
			state->iv) != 1)
		return -1;

	//printf("Key: %s iv: %s\n", state->key, state->iv);
	return 0;
}

//0 if success, -1 otherwise
//The amount of data written depends on the block alignment of the encrypted data:
//as a result the amount of data written may be anything from zero bytes to (inl + cipher_block_size - 1) so outl should contain sufficient room.
int updateEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	if (EVP_EncryptUpdate(state->ctx, outputBuffer, outputDataSize, inputBuffer,
			inputBufferSize) != 1) {
		return -1;
	}

	return 0;
}

//we (optionally) call EVP_EncryptUpdate one more time because it can make it easier to use these functions in loops
//0 if success, -1 otherwise
int finishEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	int dataSentSoFar = 0, tempint;
	//update one last time
	if (inputBufferSize > 0) {
		if (EVP_EncryptUpdate(state->ctx, outputBuffer, &dataSentSoFar,
				inputBuffer, inputBufferSize) != 1) {
			printf("error\n");
			return -1;
		}
	}

	if (EVP_EncryptFinal_ex(state->ctx, outputBuffer + dataSentSoFar, &tempint)
			!= 1)
		return -1;

	*outputDataSize = dataSentSoFar + tempint;
	/* Clean up */
	//EVP_CIPHER_CTX_free(state->ctx);
	return 0;
}

//0 if success, -1 otherwise

int startDecryption(CryptoState_t *state, const char *password, const char *iv) {

	/* get the key, iv and salt*/
	char salt[] = "12345678"; //randomly gen a salt, TODO:
	memcpy(state->salt, salt, 8);
	EVP_BytesToKey(EVP_aes_128_cbc(), EVP_sha1(), salt, password,
			strlen(password), 4, state->key, state->iv);

	/* copy the salt and iv */
	memcpy(state->salt, salt, 8);
	if (iv != NULL) {
		memcpy(state->iv, iv, IV_LENGTH);
	}

	/* init all the stuff */
	if (!(state->ctx = EVP_CIPHER_CTX_new()))
		return -1;

	if (EVP_DecryptInit_ex(state->ctx, EVP_aes_128_cbc(), NULL, state->key,
			state->iv) != 1)
		return -1;

	//printf("Key: %s iv: %s\n", state->key, state->iv);
	return 0;
}

//0 if success, -1 otherwise
//if padding is enabled the decrypted data buffer out passed to EVP_DecryptUpdate() should have sufficient room for (inl + cipher_block_size) bytes
int updateDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	if (EVP_DecryptUpdate(state->ctx, outputBuffer, outputDataSize, inputBuffer,
			inputBufferSize) != 1)
		return -1;

	return 0;
}


//we (optionally) call EVP_DecryptUpdate one more time because it can make it easier to use these functions in loops
//0 if success, -1 otherwise
int finishDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	int dataSentSoFar = 0, tempint;
	//check if we have any data waiting to be decrypted
	if (inputBufferSize > 0) {
		if (EVP_DecryptUpdate(state->ctx, outputBuffer, &dataSentSoFar,
				inputBuffer, inputBufferSize) != 1)
			return -1;
	}

	if (EVP_DecryptFinal_ex(state->ctx, outputBuffer, &tempint) != 1) {
		printf("error when calling final decrypt\n");
		return -1;
	}

	printf("done with the final decryption, the data: %d\n", tempint);
	*outputDataSize = dataSentSoFar + tempint;

	/* Clean up */
	//EVP_CIPHER_CTX_free(state->ctx);
	return 0;
}


