typedef enum {
    PROTO_HTTP,
    PROTO_HTTPS
} protocol_t;

char* get_json_value(char* inputName, char* jsonData, int jsonDataSize);

int parseUrl(char* inputUrl, protocol_t* type, char** domain, char** fileUrl);
