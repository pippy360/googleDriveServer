#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include "crypto.h"

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
int startEncryption(CryptoState_t *state, char *password, char *saltBuffer,
		const char *inputBuffer, const int inputBufferSize, char *outputBuffer,
		int *outputDataSize) {

	int encLen = 0;

	/* get the key, iv and salt*/
	saltBuffer[2] = 'a';//randomly gen a salt, TODO:
	EVP_BytesToKey(EVP_aes_128_cbc(), EVP_sha1(), saltBuffer, password,
			strlen(password), 4, state->key, state->iv);

	if (!(state->ctx = EVP_CIPHER_CTX_new()))
		return -1;

	if (EVP_EncryptInit_ex(state->ctx, EVP_aes_128_cbc(), NULL, state->key,
			state->iv) != 1)
		return -1;

	/* copy the salt to the outputBuffer and then encrypt any data in the inputBuffer */
	memcpy(outputBuffer, saltBuffer, 8);
	*outputDataSize = 8;
	if(inputBufferSize > 0){
		if (EVP_EncryptUpdate(state->ctx, outputBuffer+8, &encLen, inputBuffer,
				inputBufferSize) != 1)
			return -1;

		*outputDataSize += encLen;
	}
	return 0;
}

//0 if success, -1 otherwise
int updateEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	if (EVP_EncryptUpdate(state->ctx, outputBuffer, outputDataSize, inputBuffer,
			inputBufferSize) != 1){
		return -1;
	}

	return 0;
}

//0 if success, -1 otherwise
int finishEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	int dataSentSoFar = 0, tempint;
	//update one last time
	if(inputBufferSize > 0){
		if (EVP_EncryptUpdate(state->ctx, outputBuffer, &dataSentSoFar, inputBuffer,
				inputBufferSize) != 1){
			printf("error\n");
			return -1;
		}
	}

	if (EVP_EncryptFinal_ex(state->ctx, outputBuffer+dataSentSoFar, &tempint) != 1)
		return -1;

	printf("final called: %d\n", tempint);

	*outputDataSize = dataSentSoFar + tempint;
	/* Clean up */
	//EVP_CIPHER_CTX_free(state->ctx);
	return 0;
}

//0 if success, -1 otherwise
int startDecryption(CryptoState_t *state, char *password, char *salt,
		long byteStart, long byteEnd) {

	EVP_BytesToKey(EVP_aes_128_cbc(), EVP_sha1(), salt, password,
			strlen(password), 4, state->key, state->iv);

	if (!(state->ctx = EVP_CIPHER_CTX_new()))
		return -1;

	if (EVP_DecryptInit_ex(state->ctx, EVP_aes_128_cbc(), NULL, state->key,
			state->iv) != 1)
		return -1;


	return 0;
}

//0 if success, -1 otherwise
int updateDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	if (EVP_DecryptUpdate(state->ctx, outputBuffer, outputDataSize, inputBuffer,
			inputBufferSize) != 1)
		return -1;

	return 0;
}

//0 if success, -1 otherwise
int finishDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {

	int dataSentSoFar = 0, tempint;
	//update one last time
	if(inputBufferSize > 0){
		if (EVP_DecryptUpdate(state->ctx, outputBuffer, &dataSentSoFar, inputBuffer,
				inputBufferSize) != 1)
			return -1;
	}

	if (EVP_DecryptFinal_ex(state->ctx, outputBuffer, &tempint) != 1)
		return -1;

	*outputDataSize = dataSentSoFar + tempint;
	/* Clean up */
	//EVP_CIPHER_CTX_free(state->ctx);
	return 0;
}

int main(int argc, char **argv) {

	//NOW ENCRYPT/DECRYPT A FILE
	CryptoState_t one, two;
	CryptoState_t *enState = &one;
	CryptoState_t *deState = &two;
	char saltBuff[8];
	char encStuff[20000];
	char final[2000];
	int currDaPos = 0;
	int currEnPos = 0;
	int outputLen;
	char stuffToEn[] = "0";
	char stuffToEn2[] = "mine";
	char stuffToEn3[] = "time";
	char password[]  = "password";

	int returnCode;
	returnCode = startEncryption(enState, password, saltBuff, stuffToEn3, 0, encStuff, &outputLen);
	printf("the return code for start: %d and currPos: %d\n", returnCode, outputLen);


	returnCode = updateEncryption(enState, stuffToEn, strlen(stuffToEn), encStuff+8,
			&outputLen);
	currEnPos += outputLen;
	printf("the return code: %d and currPos: %d\n", returnCode, currEnPos);

	returnCode = updateEncryption(enState, stuffToEn2, strlen(stuffToEn2), encStuff+8 + currEnPos,
			&outputLen);
	currEnPos += outputLen;
	printf("the return code: %d and currPos: %d\n", returnCode, currEnPos);

	returnCode = finishEncryption(enState, stuffToEn3, 0, encStuff+8 + currEnPos,
			&outputLen);
	currEnPos += outputLen;
	printf("the return code: %d and currPos: %d\n", returnCode, currEnPos);

	/*      decryption         */
	currDaPos = 0;
	startDecryption(deState, password, saltBuff, 0, 0);
	updateDecryption(deState, encStuff+8, currEnPos, final, &outputLen);
	currDaPos += outputLen;
	printf("outputLen after first dec update: %d\n", outputLen);
	finishDecryption(deState, NULL, 0, final + outputLen,
			&outputLen);
	currDaPos += outputLen;
	printf("outputLen after final dec update: %d\n", outputLen);
	printf("final is: %.*s\n", currDaPos, final);
}

