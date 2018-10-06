#include "./fileTransferDriver.h"

PendingFileUpload_t *inProgressFileUploads;

PendingFileUpload_t *getUploadInProgress( long fileId ) {
	PendingFileUpload_t *curr = inProgressFileUploads;
	while ( curr ) {
		if ( curr->fileId == fileId ) {
			return curr;
		}
 		curr = inProgressFileUploads->next;
	}
	return 0;
}

int removeUploadInProgress( long fileId ) {
	PendingFileUpload_t *prev = 0;
	PendingFileUpload_t *curr = inProgressFileUploads;
	while ( curr ) {
		if ( curr->fileId == fileId ) {
			if ( !prev ) {
				inProgressFileUploads = curr->next;
				return 0;
			} 
			prev->next = curr->next;
			return 0;
		}
		prev = curr;
 		curr = inProgressFileUploads->next;
	}
	return -1;//FIXME: return doesn't exist
}

void init_PendingFileUpload( PendingFileUpload_t *pendingFileUpload, long fileId ) {
	pendingFileUpload->fileId = fileId;
	pendingFileUpload->currentBytesUploaded = 0;
}

void addUploadInProgress(PendingFileUpload_t *inFileUpload) {
	if ( !inProgressFileUploads ) {
		inProgressFileUploads = inFileUpload;
		return;
	}

	PendingFileUpload_t *curr = inProgressFileUploads;
	while ( curr->next ) {
 		curr = curr->next;
	}
	curr->next = inFileUpload;
}


