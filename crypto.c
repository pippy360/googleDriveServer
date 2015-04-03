#include "crypto.h"

//0 if success, -1 otherwise
int startDecryption(long byteStart, long byteEnd, CryptoState_t *state) {
	return 0;
}

//0 if success, -1 otherwise
int updateDecryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {
	return 0;
}

//0 if success, -1 otherwise
int finishDecryption() {
	return 0;
}

//0 if success, -1 otherwise
int startEncryption(CryptoState_t *state) {
	return 0;
}

//0 if success, -1 otherwise
int updateEncryption(CryptoState_t *state, const char *inputBuffer,
		const int inputBufferSize, char *outputBuffer, int *outputDataSize) {
	return 0;
}

//0 if success, -1 otherwise
int finishEncryption() {
	return 0;
}

int main(int argc, char **argv) {

	//test encrypting then decrypting it!
}

