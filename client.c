/*
** client.c -- A client for the chatroom
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define NAME_SIZE 32

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_SIZE];

/********** Function Prototypes **********/
/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 24, 2023
 * Desc: Creates input section on stdout for typing
*/
void text_prompt_stdout();

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 24, 2023
 * Desc: Trims
*/
void str_trim_lf(char* arr, int length);

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 24, 2023
 * Desc: Catches ctrl+c and exit to leave chatroom
*/
void leave_chatroom_signal();

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 23, 2023
 * Desc: Gets the socket address, IPv4 or IPv6
*/
void* get_in_addr(struct sockaddr* sa);

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 23, 2023
 * Desc: Creates thread to handle messages received
*/
void recv_msg_handler();

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 23, 2023
 * Desc: Creates thread to handle messages sent
*/
void send_msg_handler();


/***********Main***********/
/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: February 8, 2023
 * Desc: Main - Runs the server for the chatroom
*/
int main(int argc, char* argv[]) {

    /* Check for usage */
    if (argc != 2) {
        fprintf(stdout, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* ip = "127.0.0.1";
    char* port = argv[1];
    struct addrinfo hints;
    struct addrinfo* servinfo;
    struct addrinfo* serv_ptr;
    int rv;

    signal(SIGINT, leave_chatroom_signal);
    
    fprintf(stdout, "Enter your name: ");
    fgets(name, NAME_SIZE, stdin);
    str_trim_lf(name, strlen(name));

    if (strlen(name) >  NAME_SIZE - 1 || strlen(name) < 2) {
        fprintf(stdout, "Name format incorrect\n");
        return EXIT_FAILURE;
    }

    /* Socket Settings */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Error: getaddrinfo - %s\n", gai_strerror(rv));
        return EXIT_FAILURE;
    }

    for (serv_ptr = servinfo; serv_ptr != NULL; serv_ptr = serv_ptr->ai_next) {
        if ((sockfd = socket(serv_ptr->ai_family, serv_ptr->ai_socktype, serv_ptr->ai_protocol)) == -1) {
            perror("Error: Client - socket");
            continue; /* Check next address */
        }

        if (connect(sockfd, serv_ptr->ai_addr, serv_ptr->ai_addrlen) == -1) {
            close(sockfd);
            perror("Error: Client - connect");
            continue;
        }

        break;
    }

    if (serv_ptr == NULL) {
        fprintf(stderr, "Error: Client failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo); /* Free up the linked list */

    /* Send the name */
    send(sockfd, name, NAME_SIZE, 0);
    puts("***** WELCOME TO LiNNNk'S CHATROOM *****");

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0) {
        fprintf(stderr, "Error: Client - pthread\n");
        return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0) {
        fprintf(stderr, "Error: Client - pthread\n");
        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            fprintf(stdout, "\nBye\n");
            break;
        }

    }
    close(sockfd);

    return EXIT_SUCCESS;
}


/********** Function Definitions **********/
void text_prompt_stdout() {
    fprintf(stdout, "\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char* arr, int length) {
    int i;

    for (i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void leave_chatroom_signal() {
    flag = 1;
}

void* get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void recv_msg_handler() {
    char message[BUFFER_SIZE] = {};

    while (1) {
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);
        
        if (receive > 0) {
            fprintf(stdout, "%s ", message);
            text_prompt_stdout();
        } else if (receive == 0) {
            break;
        }

        memset(message, 0, sizeof(message));
    }
}

void send_msg_handler() {
    char buffer[BUFFER_SIZE] = {};
    char message[BUFFER_SIZE + NAME_SIZE + 4] = {};

    while (1) {
        text_prompt_stdout();
        fgets(buffer, BUFFER_SIZE, stdin);
        str_trim_lf(buffer, BUFFER_SIZE);

        if (strncmp(buffer, "exit", strlen("exit") == 0)) {
            break;
        } else {
            sprintf(message, "%s: %s\n", name, buffer);
            send(sockfd, message, strlen(message), 0);
        }

        memset(buffer, 0, sizeof(buffer));
        memset(message, 0, sizeof(message));
    }
    leave_chatroom_signal(2);
}
   