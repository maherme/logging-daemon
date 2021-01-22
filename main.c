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

#define SERVER_PATH "/dev/log"
#define BUFLEN 1024

static int CreateSocket(void);
static void BindSocketServer(int);
static void ReadSocket(int, int, char **);
static void WriteFiles(int, char **, char *);
static void Exception(int);

static volatile bool loop_cond = true;

int main(int argc, char *argv[]){

    int sockfd = -1;

    if(argc < 2){
        perror("Error: missing file or files as an argument");
        exit(-1);
    }

    if(signal(SIGINT, Exception) == SIG_ERR){
        perror("Error: SIGINT subscription failed");
	exit(-1);
    }

    sockfd = CreateSocket();

    BindSocketServer(sockfd);

    ReadSocket(sockfd, argc, argv);

    close(sockfd);

    return 0;
}

/* Function in charge of creating a socket.
 * Socket shall be configured as non blocking.
 */
static int CreateSocket(void){

    int sock_addr = -1;

    if((sock_addr = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
        perror("Error: socket could not be created");
	exit(-1);
    }

    if(fcntl(sock_addr, F_SETFL, O_NONBLOCK) < 0){
	perror("Error: fcntl could not set non blocking operations");
	exit(-1);
    }

    return sock_addr;
}

/* Function to bind socket to the server address.
 * Set socket perms to read/write for anybody.
 */
static void BindSocketServer(int fd){

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, SERVER_PATH, sizeof(SERVER_PATH));

    if((bind(fd, (struct sockaddr *)&addr, SUN_LEN(&addr))) < 0){
        perror("Error: bind failed");
        exit(-1);
    }

    chmod(SERVER_PATH, S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
}

/* Function to read data from socket and storing in files
 * fd is the socket descripton
 * n_files is the number of destination files
 * files_names is an array with the file names
 */
static void ReadSocket(int fd, int n_files, char **files_names){

    int ret_select = 0;
    int len_rec = 0;
    int addr_client_len = 0;
    char buf[BUFLEN] = {0};
    char buf_old[BUFLEN] = {0};
    fd_set socket_set;
    struct sockaddr_un addr_client;
    struct timeval timeout;

    addr_client_len = SUN_LEN(&addr_client);

    FD_ZERO(&socket_set);
    FD_SET(fd, &socket_set);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while(loop_cond == true){

	ret_select = select(fd+1, &socket_set, NULL, NULL, &timeout);
	if((ret_select < 0) && (errno != EINTR)){
	    perror("Error: select function return error");
	    exit(-1);
	}
	else if(ret_select > 0){
            if(((len_rec = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&addr_client, &addr_client_len)) < 0) && (errno != EWOULDBLOCK)){
	        perror("Error: reception from client");
	        exit(-1);
	    }
	    else if(len_rec > 0){
	        buf[len_rec] = '\n';
                printf("%s", buf);
		/* Avoid duplicated logs */
		if(strcmp(buf, buf_old) == 0){
		    printf("Log duplicated\r\n");
		}
		else{
		    WriteFiles(n_files, files_names, buf);
		}
		strcpy(buf_old, buf);
		/* Clean buffer before the next usage */
	        memset(buf, 0, sizeof(buf));
            }
	    else{
		/* do nothing */
	    }
	    FD_SET(fd, &socket_set);
	    timeout.tv_sec = 5;
	    timeout.tv_usec = 0;
	}
	else{
	    FD_SET(fd, &socket_set);
	    timeout.tv_sec = 5;
	    timeout.tv_usec = 0;
	}
    }
}

/* Function to write buffer content in all files passed as arguments.
 * number_files is the number of files for writing.
 * files_names contains the name of the files.
 */
static void WriteFiles(int number_files, char **files_names, char *buffer){

    int i = 0;
    FILE *filed = NULL;

    for(i = 0; i< (number_files-1); i++){
        if((filed = fopen(files_names[i+1], "a")) == NULL){
            perror("Error: failed opening a file");
            exit(-1);
        }
        fprintf(filed, buffer);
        if(fclose(filed) < 0){
            perror("Error: failed closing a file");
            exit(-1);
        }
    }

}

/* Function subscribed to SIGINT (ctrl-c) signal.
 * Break the loop condition to exit from main in the right way.
 */
static void Exception(int num_signal){

    loop_cond = false; /* Break the loop condition */
    signal(SIGINT, SIG_DFL);
}
