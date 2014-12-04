typedef enum {
	start,
	sendingHeader,
	sendingMetaData,
	sendingFileDataDelimiter,
	sendingFileData,
	sendingEndDelimiter,
	finished,
	ERROR//this might not be needed
} googleUploadState;

typedef struct {
	unsigned int  stringOffset;//used to keep track of how much of the header/metadata we've sent
	unsigned int  headerRemainingData;
	unsigned long fileRemainingData;
	googleUploadState state;
} googleUploadStruct;



