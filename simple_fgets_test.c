#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFERSIZE 100

int main()
{
	char *text = calloc(1,1), buffer[BUFFERSIZE];
	printf("Enter a message: \n");
	fgets(buffer, BUFFERSIZE , stdin); /* break with ^D or ^Z */

	printf("\ntext:\n%s",buffer);
}