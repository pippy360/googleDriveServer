#define MAX_PACKET_LENGTH 1600







typedef enum {
	REQUEST_ABOR,
	REQUEST_ACCT,
	REQUEST_ALLO,
	REQUEST_APPE,
	REQUEST_CWD,
	REQUEST_CDUP,
	REQUEST_DELE,
	REQUEST_LIST,
	REQUEST_MKD,
	REQUEST_MDTM,
	REQUEST_MODE,
	REQUEST_NLST,
	REQUEST_NOOP,
	REQUEST_PASS,
	REQUEST_PASV,
	REQUEST_PORT,
	REQUEST_PWD,
	REQUEST_QUIT,
	REQUEST_RETR,
	REQUEST_RMD,
	REQUEST_RNFR,
	REQUEST_RNTO,
	REQUEST_SITE,
	REQUEST_UMASK,
	REQUEST_IDLE,
	REQUEST_CHMOD,
	REQUEST_HELP,
	REQUEST_SIZE,
	REQUEST_STAT,
	REQUEST_STOR,
	REQUEST_STOU,
	REQUEST_STRU,
	REQUEST_SYST,
	REQUEST_TYPE,
	REQUEST_USER,
	REQUEST_XCUP,
	REQUEST_XCWD,
	REQUEST_XMKD,
	REQUEST_XPWD,
	REQUEST_XRMD,
	REQUEST_ERROR
} ftpRequest_t;

typedef enum {
	FTP_TRANSFER_TYPE_A,
	FTP_TRANSFER_TYPE_E,
	FTP_TRANSFER_TYPE_I,
	FTP_TRANSFER_TYPE_L
} ftpTransferType_t;

typedef struct {
	ftpRequest_t type;
	char *paramBuffer;
	int paramBufferLength;
} ftpParserState_t;

typedef struct {
	int command_fd;
	int data_fd;
	int data_fd2;
	char isDataConnectionOpen;
	char isAwaitingRename;
	char *usernameBuffer;
	int usernameBufferLength;
	char *fileNameChangeBuffer;
	int fileNameChangeBufferLength;
	int loggedIn;
	int cwdId;
	ftpTransferType_t transferType;
} ftpClientState_t;








