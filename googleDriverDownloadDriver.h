#ifndef GOOGLEDRIVEDOWNLOADDRIVER_H
#define GOOGLEDRIVEDOWNLOADDRIVER_H

typedef struct {
	.prepareDriver = ,
	.startDownload = ,
	.updateDownload = ,
	.finishDownload = , 
} DownloadDriver_ops_t;

typedef struct {
	DownloadDriver_ops_t ops,
	void *priv,
} DownloadDriver_t;


#endif /* GOOGLEDRIVEDOWNLOADDRIVER_H */