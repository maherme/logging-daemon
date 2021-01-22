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
#define DATE_LOG_LEN 20

static int CreateSocket(void);
static void BindSocketServer(int);
static void ReadSocket(int, int, char **);
static int WriteFiles(int, char **, char *);
static void MostRepeatedLog(char *);
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

    MostRepeatedLog(argv[1]);

    return 0;
}

/* Function in charge of creating a socket.
 * Socket shall be configured as non blocking.
 *
 * Return: socket descriptor.
 */
static int CreateSocket(void){

    int sock_addr = -1;

    if((sock_addr = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
        perror("Error: socket could not be created");
	exit(-1);
    }

    if(fcntl(sock_addr, F_SETFL, O_NONBLOCK) < 0){
	perror("Error: fcntl could not set non blocking operations");
	close(sock_addr);
	exit(-1);
    }

    return sock_addr;
}

/* Function to bind socket to the server address.
 * Set socket perms to read/write for anybody.
 *
 * Parameters:
 * fd: socket descriptor.
 */
static void BindSocketServer(int fd){

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, SERVER_PATH, sizeof(SERVER_PATH));

    if((bind(fd, (struct sockaddr *)&addr, SUN_LEN(&addr))) < 0){
        perror("Error: bind failed");
	close(fd);
        exit(-1);
    }

    chmod(SERVER_PATH, S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
}

/* Function to read data from socket and storing in files
 *
 * Parameters:
 * fd: socket descriptor.
 * n_files: is the number of destination files.
 * files_names: is an array with the file names.
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
	    close(fd);
	    exit(-1);
	}
	else if(ret_select > 0){
            if(((len_rec = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&addr_client, &addr_client_len)) < 0) && (errno != EWOULDBLOCK)){
	        perror("Error: reception from client");
		close(fd);
	        exit(-1);
	    }
	    else if(len_rec > 0){
	        buf[len_rec] = '\n';
                printf("%s", buf);
		/* Avoid duplicated logs */
		if(strcmp(buf, buf_old) == 0){
		    printf("Warning: log duplicated\n");
		}
		else{
		    if(WriteFiles(n_files, files_names, buf) < 0){
			exit(-1);
		    }
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
 *
 * Parameters:
 * number_files: is the number of files for writing.
 * files_names: contains the name of the files.
 * buffer: data source.
 *
 * Return: -1 if an error ocurred, 0 if completed OK.
 */
static int WriteFiles(int number_files, char **files_names, char *buffer){

    int ret = 0;
    int i = 0;
    FILE *filed = NULL;

    for(i = 0; i< (number_files-1); i++){
        if((filed = fopen(files_names[i+1], "a")) == NULL){
            perror("Error: failed opening a file");
            ret = -1;
        }
        fprintf(filed, buffer);
        if(fclose(filed) < 0){
            perror("Error: failed closing a file");
            ret = -1;
        }
    }

    return ret;
}

/* Function to count the number of repeated log messages in a file.
 * It parses rows from input file (avoiding date field) to temp file.
 * It sorts the message from temp file to temp2 file.
 * It counts the number of repeated messages and updates an structure
 * with the most repeated log message and the count.
 *
 * Parameters:
 * file_name: contains the name of the target file.
 */
static void MostRepeatedLog(char *file_name){

    int count = 1;
    FILE *fd = NULL;
    FILE *temp = NULL;
    FILE *temp2 = NULL;
    char buf[BUFLEN] = {0};
    char buf_old[BUFLEN] = {0};
    struct most_repeated{
	char log_message[BUFLEN];
	int times;
    } log_most_rep;

    if((fd = fopen(file_name, "r")) == NULL){
	perror("Error: failed opening a file");
	exit(-1);
    }

    if((temp = fopen("/tmp/temp_log0.log", "w")) == NULL){
	perror("Error: failed opening a file");
	fclose(fd);
	exit(-1);
    }

    if((temp2 = fopen("/tmp/temp_log1.log", "w")) == NULL){
	perror("Error: failed opening a file");
	fclose(fd);
	fclose(temp);
	exit(-1);
    }

    /* Parsing of rows avoiding the date field (DATE_LOG_LEN offset) */
    while(fgets(buf, BUFLEN, fd) != NULL){
	fputs(buf+DATE_LOG_LEN, temp2);
    }

    if((fclose(fd) < 0) || (fclose(temp) < 0) || (fclose(temp2) < 0)){
	perror("Error: failed closing a file");
	exit(-1);
    }

    if(system("sort -k 2 /tmp/temp_log1.log > /tmp/temp_log0.log") < 0){
	perror("Error: system call failed");
	exit(-1);
    }

    if(remove("/tmp/temp_log1.log") < 0){
        perror("Error: /tmp/temp_log1.log can not be deleted");
    }

    if((temp = fopen("/tmp/temp_log0.log", "r")) == NULL){
	perror("Error: failed opening a file");
	exit(-1);
    }

    /* Initiating structure */
    log_most_rep.times = 0;
    memset(log_most_rep.log_message, 0, sizeof(log_most_rep.log_message));

    while(fgets(buf, BUFLEN, temp) != NULL){
	if(buf_old[0] == '\0'){
	    strcpy(buf_old, buf);
	}
	else{
	    if(strcmp(buf_old, buf) == 0){
	        count++;
	    }
	    else{
		if(count > log_most_rep.times){
		    log_most_rep.times = count;
		    strcpy(log_most_rep.log_message, buf_old);
		}
		strcpy(buf_old, buf);
		count = 1;
	    }
	}
    }
    /* Checking the last log message read */
    if(count > log_most_rep.times){
	log_most_rep.times = count;
	strcpy(log_most_rep.log_message, buf_old);
    }

    if(fclose(temp) < 0){
	perror("Error: failed closing a file");
	exit(-1);
    }

    if(remove("/tmp/temp_log0.log") < 0){
        perror("Error: /tmp/temp_log0.log can not be deleted");
    }

    printf("%d --> %s", log_most_rep.times, log_most_rep.log_message);
}

/* Function subscribed to SIGINT (ctrl-c) signal.
 * Break the loop condition to exit from main in the right way.
 */
static void Exception(int num_signal){

    loop_cond = false; /* Break the loop condition */
    signal(SIGINT, SIG_DFL);
}
