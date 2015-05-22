/**
 AES encryption/decryption demo program using OpenSSL EVP apis
 gcc -Wall openssl_aes.c -lcrypto

 this is public domain code.

 Saju Pillai (saju.pillai@gmail.com)
 **/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>

/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt,
		EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx) {
	int i, nrounds = 5;
	unsigned char key[32], iv[32];

	/*
	 * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
	 * nrounds is the number of times the we hash the material. More rounds are more secure but
	 * slower.
	 */
	i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data,
			key_data_len, nrounds, key, iv);
	if (i != 32) {
		printf("Key size is %d bits - should be 256 bits\n", i);
		return -1;
	}

	EVP_CIPHER_CTX_init(e_ctx);
	EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
	EVP_CIPHER_CTX_init(d_ctx);
	EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

	return 0;
}

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext,
		int *len) {
	/* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
	int c_len = *len + 16, f_len = 0;
	unsigned char *ciphertext = malloc(c_len);

	/* allows reusing of 'e' for multiple encryption cycles */
	EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);

	/* update ciphertext, c_len is filled with the length of ciphertext generated,
	 *len is the size of plaintext in bytes */
	EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);

	/* update ciphertext with the final remaining bytes */
	EVP_EncryptFinal_ex(e, ciphertext + c_len, &f_len);

	*len = c_len + f_len;
	return ciphertext;
}

/*
 * Decrypt *len bytes of ciphertext
 */
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext,
		int *len) {
	/* plaintext will always be equal to or lesser than length of ciphertext*/
	int p_len = *len, f_len = 0;
	unsigned char *plaintext = malloc(p_len);

	EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
	EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
	EVP_DecryptFinal_ex(e, plaintext + p_len, &f_len);

	*len = p_len + f_len;
	return plaintext;
}
void handleErrors() {

}

int main(int argc, char **argv) {
	/* "opaque" encryption, decryption ctx structures that libcrypto uses to record
	 status of enc/dec operations */

	EVP_CIPHER_CTX *ctx;
	char key[32];
	char iv[16] = "";

	int  someint, newSomeInt;
	char server[10000];
	char password[]  = "phone";
	char plainText[] = "0123456789012345";
//	char plainText[] = "sosdafsdafsdafdsafdsafdssss-----------------------------------afsdaf_";
	/* Initialise the library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);

	EVP_BytesToKey(EVP_aes_128_xts(), EVP_sha1(), "12345678", "phone",
				strlen("phone"), 4, key, iv);
	ctx = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx, EVP_aes_128_xts(), NULL, key, iv);

	//check if key and iv match
	printf("key: --%.*s--\n", 32, key);
	printf("iv : --%.*s--\n", 16, iv );

	EVP_EncryptUpdate(ctx, server, &someint, plainText,
					strlen(plainText)+1);

	printf("after first update: %s\n", server);
	printf("stats after first, offset: %d\n", someint);

	if (1 != EVP_EncryptFinal_ex(ctx, server + someint, &newSomeInt))
		printf("error\n");

	printf("final called, value is: %d\n", newSomeInt);
	someint += newSomeInt;

	printf("after final update: %s\n", server);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	printf("someint: %d\n", someint);

	printf("Ciphertext is:\n");
	BIO_dump_fp(stdout, server, someint);


	EVP_CIPHER_CTX *ctxNew;
	char output[10000];
	int cryptoLen, newCryptoLen;

	ctxNew = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctxNew, EVP_aes_128_xts(), NULL, key, iv);

	EVP_DecryptUpdate(ctx, output, &cryptoLen, server, someint);

	EVP_DecryptFinal_ex(ctx, output + cryptoLen, &newCryptoLen);

	cryptoLen += newCryptoLen;

	printf("someint: %d\n", someint);
	printf("crypto: %d\n", cryptoLen);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctxNew);

	printf("output %d: %.*s\n", cryptoLen, cryptoLen, output);
}

