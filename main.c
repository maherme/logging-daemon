#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

#define SERVER_PATH "/dev/log"
#define BUFLEN 1024

int main(int argc, char *argv[]){

    int sockfd = -1;
    int ret_read = 0;
    int addr_client_len = 0;
    struct sockaddr_un addr_server, addr_client;
    char buf[BUFLEN] = {0};

    addr_client_len = SUN_LEN(&addr_client);

    if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
        perror("Error: socket could not be created");
	exit(-1);
    }

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    memcpy(addr_server.sun_path, SERVER_PATH, sizeof SERVER_PATH);

    if((bind(sockfd, (struct sockaddr *)&addr_server, SUN_LEN(&addr_server))) < 0){
        perror("Error: bind failed");
        exit(-1);
    }

    for(;;){
	if((recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&addr_client, &addr_client_len)) < 0){
	    perror("Error: reception from client");
	    exit(-1);
	}
	printf("%s\n", buf);
    }

    close(sockfd);

    return 0;
}
