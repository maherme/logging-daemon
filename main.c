#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define SERVER_PATH "/dev/log"
#define BUFLEN 1024

int main(int argc, char *argv[]){

    int i = 0;
    int sockfd = -1;
    int ret_read = 0;
    int addr_client_len = 0;
    int len_rec = 0;
    FILE* filed = NULL;
    struct sockaddr_un addr_server, addr_client;
    char buf[BUFLEN] = {0};

    if(argc < 2){
        perror("Error: missing file or files as an argument");
        exit(-1);
    }

    addr_client_len = SUN_LEN(&addr_client);

    if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
        perror("Error: socket could not be created");
	exit(-1);
    }

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    memcpy(addr_server.sun_path, SERVER_PATH, sizeof(SERVER_PATH));

    if((bind(sockfd, (struct sockaddr *)&addr_server, SUN_LEN(&addr_server))) < 0){
        perror("Error: bind failed");
        exit(-1);
    }

    chmod(SERVER_PATH, S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

    for(;;){
	if((len_rec = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&addr_client, &addr_client_len)) < 0){
	    perror("Error: reception from client");
	    exit(-1);
	}
	else{
	    buf[len_rec] = '\n';
            printf("%s", buf);

	    for(i = 0; i< (argc-1); i++){
		if((filed = fopen(argv[i+1], "a")) == NULL){
		    perror("Error: failed opening a file");
		    exit(-1);
		}
	        fprintf(filed, buf);
		if(fclose(filed) < 0){
		    perror("Error: failed closing a file");
		    exit(-1);
		}
	    }
	    memset(buf, 0, sizeof(buf));
        }
    }

    close(sockfd);

    return 0;
}
