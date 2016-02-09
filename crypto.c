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
int startEncryption(CryptoState_t *state, char *password) {

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

int startDecryption(CryptoState_t *state, char *password, char *iv) {

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

/*
 * LET'S ENCRYPT OUTPUT3.WEBM
 * */
/*
int main(int argc, char **argv) {

	//NOW ENCRYPT/DECRYPT A FILE
	CryptoState_t one, two;
	CryptoState_t *enState = &one;
	CryptoState_t *deState = &two;
	char saltBuff[8];
	char encStuff[20000];
	char final[2000];
	int outputLen;
	char password[] = "password";


	char* inputFile = argv[1];
	char ch;
	char readBuffer[1000];
//	char readBuffer[] = "tom was here doing this and i'm ok with it test test stes test est est ";
	char cryptoBuffer[1000];
	FILE *txtFp, *cryptoFp;

	txtFp = fopen(inputFile, "r");
	if (txtFp == NULL && cryptoFp == NULL) {
		perror("Error while opening the file.\n");
		exit(EXIT_FAILURE);
	}

	//read in 100 bytes or
	int readSize = 900;
	int cryptoSize = 0;

	startEncryption(enState, "phone");
	cryptoFp = fopen("outputCrypto", "w+");

	int bytesRead;
	while ((bytesRead = fread(readBuffer, 1, readSize, txtFp)) > 0) {
		updateEncryption(enState, readBuffer, bytesRead, cryptoBuffer,
				&outputLen);
		fwrite(cryptoBuffer, outputLen, 1, cryptoFp);
		cryptoSize += outputLen;
	}
	if (!feof(txtFp)) {
		perror("we got some error while reading the file O_o");
	}

	finishEncryption(enState, NULL, 0, cryptoBuffer, &outputLen);
	fwrite(cryptoBuffer, outputLen, 1, cryptoFp);
	cryptoSize += outputLen;
	printf("done: %d, %d\n", cryptoSize, outputLen);

	fclose(cryptoFp);
	fclose(txtFp);

	printf("finished encryption\n");

	//^ done part one
	//now open and decrypt

	int startPos = 0;
	int endPos = 0;
	int chunkSize = 500; //we will read the crypted file in chunks
	int blockSize = 16;

	int actStart = startPos - (startPos % blockSize);
	int actEnd =
			(endPos % blockSize == 0) ?
					endPos : endPos + (blockSize - (endPos % blockSize));

	int newReadSize = (actEnd - actStart);
	char inputBuffer[1000];
	char outputBuffer[1000];
	int newBytesRead;
	FILE *orgCrypto, *finalOutput;

	printf("start: %d, end: %d\n", actStart, actEnd);

	orgCrypto = fopen("outputCrypto", "r");
	finalOutput = fopen("finalOutput", "w+");
	//seek to where the download would start

	//read the first chunk
	newBytesRead = fread(inputBuffer, 1, chunkSize, orgCrypto);

	if (actStart <= 0) {
		startDecryption(deState, "phone", NULL);
	} else {
		startDecryption(deState, "phone", inputBuffer);
	}

	updateDecryption(deState, inputBuffer, newBytesRead, outputBuffer, &outputLen);
	fwrite(outputBuffer, outputLen, 1, finalOutput);

	//while until we have finished reading the file or until we've processed enough data to hit our read limit
	while ((newBytesRead = fread(inputBuffer, 1, chunkSize, orgCrypto)) > 0) {
		updateDecryption(deState, inputBuffer, newBytesRead, outputBuffer,
				&outputLen);

		fwrite(outputBuffer, outputLen, 1, finalOutput);
	}
	if (!feof(orgCrypto)) {
		perror("NO.2 we got some error while reading the file O_o");
	}
	finishDecryption(deState, NULL, 0, outputBuffer, &outputLen);
	fwrite(outputBuffer, outputLen, 1, finalOutput);

	//close all the files
	close(orgCrypto);
	close(finalOutput);
	return 0;
}
*/
/* SOME TEST CASES
 int main(int argc, char **argv) {

 //NOW ENCRYPT/DECRYPT A FILE
 CryptoState_t one, two;
 CryptoState_t *enState = &one;
 CryptoState_t *deState = &two;
 char saltBuff[8];
 char encStuff[20000];
 char final[2000];
 int outputLen;
 char password[] = "password";

 char ch;
 char readBuffer[1000];
 //	char readBuffer[] = "tom was here doing this and i'm ok with it test test stes test est est ";
 char cryptoBuffer[1000];
 FILE *txtFp, *cryptoFp;

 txtFp = fopen("partialTest.txt", "r");
 cryptoFp = fopen("crypto.txt", "w+");

 if (txtFp == NULL && cryptoFp == NULL) {
 perror("Error while opening the file.\n");
 exit(EXIT_FAILURE);
 }

 //read in 100 bytes or
 int readSize = 1000;
 int cryptoSize = 0;
 fread(readBuffer, readSize, 1, txtFp);

 startEncryption(enState, "phone");

 updateEncryption(enState, readBuffer, readSize, cryptoBuffer, &outputLen);
 cryptoSize += outputLen;

 finishEncryption(enState, NULL, 0, cryptoBuffer+outputLen, &outputLen);
 cryptoSize += outputLen;

 cryptoFp = fopen("crypto.txt", "w+");
 fwrite(cryptoBuffer, cryptoSize, 1, cryptoFp);

 fclose(cryptoFp);
 fclose(txtFp);

 //^ done part one

 int startPos = 30;
 int endPos   = 100;
 int blockSize = 16;

 int actStart 	=  startPos - (startPos%blockSize);
 int actEnd 		=  (endPos%blockSize == 0)? endPos: endPos + (blockSize - (endPos%blockSize));
 int newReadSize  	= (actEnd - actStart);

 printf("start: %d, end: %d\n", actStart, actEnd);

 char inputBuffer[1000];
 char outputBuffer[1000];

 txtFp = fopen("crypto.txt", "r");
 fseek( txtFp, (long) (actStart - blockSize), SEEK_SET );
 fread(inputBuffer, newReadSize, 1, txtFp);

 if(actStart <= 0){
 startDecryption(deState, "phone", NULL);
 }else{
 startDecryption(deState, "phone", inputBuffer );
 }

 updateDecryption(deState, inputBuffer + blockSize, newReadSize - blockSize, outputBuffer, &outputLen);

 finishDecryption(deState, NULL, 0, cryptoBuffer+outputLen, &outputLen);

 printf("final result: %s\n", outputBuffer+(startPos-actStart));

 //printf("readBuffer of size %d: %.*s\n", cryptoSize, outputLen, readBuffer);
 return 0;
 }
 */
