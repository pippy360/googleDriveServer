#ifndef DOWNLOADDRIVER_H
#define DOWNLOADDRIVER_H

typedef struct {
	int (*prepareDriver)(int);
	int (*startDownload)(int);
	int (*updateDownload)(int);
	//@finishDownload doesn't return any data only cleans up
	int (*finishDownload)(int);
	char *(*getExtraHeaders)();
} DownloadDriver_ops_t;

typedef struct {
	DownloadDriver_ops_t ops;
	void *priv;
} DownloadDriver_t;

typedef struct {
	DownloadDriver_t driver;
	CryptoState_t *encryptionState;
	Connection_t *con;
} FileDownload_t;

typedef struct {
	DownloadDriver_t driver;
	CryptoState_t *encryptionState;
	Connection_t *con;
} FileUpload_t;

#endif /* DOWNLOADDRIVER_H */
