#ifndef FTPPARSER_H
#define FTPPARSER_H

void ftp_newParserState( ftpHTTPParserState_t *state, char *paramBuffer, int paramBufferLength );

int ftp_parsePacket( char *packet, int packetLength, ftpHTTPParserState_t *parserState, 
		ftpClientState_t *clientState );

#endif /* FTPPARSER_H */