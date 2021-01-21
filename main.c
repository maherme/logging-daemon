#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>

#define TRUE 1
#define FALSE 0
#define SERVER_PATH "/dev/log"
#define BUFLEN 1024

void exception(int);

static volatile bool loop_cond = true;

int main(int argc, char *argv[]){

    int i = 0;
    int ret_read = 0;
    int addr_client_len = 0;
    int len_rec = 0;
    int sockfd = -1;
    int ret_select = 0;
    FILE* filed = NULL;
    struct sockaddr_un addr_server, addr_client;
    struct timeval timeout;
    char buf[BUFLEN] = {0};
    fd_set socket_set;

    if(argc < 2){
        perror("Error: missing file or files as an argument");
        exit(-1);
    }

    if(signal(SIGINT, exception) == SIG_ERR){
        perror("Error: SIGINT subscription failed");
	exit(-1);
    }

    addr_client_len = SUN_LEN(&addr_client);

    if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
        perror("Error: socket could not be created");
	exit(-1);
    }

    if(fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0){
	perror("Error: fcntl could not set non blocking operations");
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

    FD_ZERO(&socket_set);
    FD_SET(sockfd, &socket_set);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while(loop_cond == true){
	ret_select = select(sockfd+1, &socket_set, NULL, NULL, &timeout);
	if((ret_select < 0) && (errno != EINTR)){
	    perror("Error: select function return error");
	    exit(-1);
	}
	else if(ret_select > 0){
            if(((len_rec = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&addr_client, &addr_client_len)) < 0) && (errno != EWOULDBLOCK)){
	        perror("Error: reception from client");
	        exit(-1);
	    }
	    else if(len_rec > 0){
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
	    else{
		/* do nothing */
	    }
	    timeout.tv_sec = 5;
	    timeout.tv_usec = 0;
	}
	else{
	    FD_SET(sockfd, &socket_set);
	    timeout.tv_sec = 5;
	    timeout.tv_usec = 0;
	}
    }

    close(sockfd);

    return 0;
}

void exception(int num_signal){
    loop_cond = false;
    signal(SIGINT, SIG_DFL);
}
