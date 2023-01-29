/*
** server.c -- A stream socket server for a chatroom
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
#define BACK_LOGS 10

static _Atomic unsigned int client_count = 0; /* Need to review _Atomic, unsure on what it really does */
static int uid = 10;

/* Client Structure */
typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[INET6_ADDRSTRLEN];
} client_t;

client_t* clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/********** Function Prototypes **********/
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
 * Desc: Gets the socket port
*/
int get_in_port(struct sockaddr* sa);

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
 * Desc: Add a client to the chatroom queue
*/
void add_client(client_t* client);

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 28, 2023
 * Desc: Remove a client to the chatroom queue
*/
void remove_client(int uid);

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 28, 2023
 * Desc: Sends messages to clients in chatroom
*/
void send_message(char* msg, int uid);

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 28, 2023
 * Desc: Handles the clients when joining the queue
*/
void* handle_client(void* arg);

/***********Main***********/
/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 23, 2023
 * Desc: Main - Runs the server for the chatroom
*/
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stdout, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* port = argv[1];
    char client_addr_str[INET6_ADDRSTRLEN];
    int yes = 1;
    int serverfd = 0;
    int clientfd = 0;
    int rv = 0;
    struct addrinfo hints;
    struct addrinfo* servinfo;
    struct addrinfo* serv_ptr;
    struct sockaddr_storage client_addr;
    socklen_t client_size;
    pthread_t tid;

    /* Socket Setting */
    memset(&hints, 0, sizeof hints);
    hints.ai_addr = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Error: getaddrinfo - %s\n", gai_strerror(rv));
        return EXIT_FAILURE;
    }

    for (serv_ptr = servinfo; serv_ptr != NULL; serv_ptr = serv_ptr->ai_next) {
        if ((serverfd = socket(serv_ptr->ai_family, serv_ptr->ai_socktype, serv_ptr->ai_protocol)) == -1) {
            perror("Error: socket");
            continue; /* Check next address */
        }

        /* Signals */
        signal(SIGPIPE, SIG_IGN); /* sigaction would probs be better...need to revisit this */

        if (setsockopt(serverfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &yes, sizeof(int)) == -1) {
            perror("Error: setsockopt");
            exit(EXIT_FAILURE);
        }
        
        /* Bind */
        if (bind(serverfd, serv_ptr->ai_addr, serv_ptr->ai_addrlen) == -1) {
            close(serverfd);
            perror("Error: unable to bind socket...will try next one if any");
            continue;
        } 

        break; /* Break loop, so we don't continue to cycle through addresses */         
    } /* End of for-loop */

    freeaddrinfo(servinfo); /* Shouldn't need the linked list again, so free it up */
    if (serv_ptr == NULL) {
        fprintf(stderr, "Error: server failed to bind\n");
        exit(EXIT_FAILURE);
    }

    /* Listen */
    if (listen(serverfd, BACK_LOGS) == -1) {
        perror("Error: listen");
        exit(EXIT_FAILURE);
    }

    puts("***** WELCOME TO LINK'S CHATROOM *****");

    /* Infinite Loop - Essentially Chatroom engine */
    while (1) {
        client_size = sizeof client_addr;
        clientfd = accept(serverfd, (struct sockaddr*)&client_addr, &client_size);
        if (clientfd == -1) {
            perror("Error: unable to accept client");
            continue;
        }

        inet_ntop(
            client_addr.ss_family, 
            get_in_addr((struct sockaddr*)&client_addr), 
            client_addr_str, 
            sizeof client_addr_str
        );

        /* Check for the maximum clients */
        if ((client_count + 1) == MAX_CLIENTS) {
            fprintf(stdout, 
                    "Max clients connected. Connection Rejected: %s: %d\n", 
                    client_addr_str, 
                    get_in_port((struct sockaddr*)&client_addr));

            close(clientfd);
            continue;
        }

        /* Client Settings */
        client_t* client = (client_t*)malloc(sizeof(client_t));
        client->address = *((struct sockaddr_in*)(get_in_addr((struct sockaddr*)&client_addr)));
        client->sockfd = clientfd;
        client->uid = uid++;

        /* Add Client to the queue */
        add_client(client);
        pthread_create(&tid, NULL, &handle_client, (void*)client);

        /* Tutorial says Reduce CPU Usage...not sure what this means when we're just sleeping */
        sleep(1);

    }


    return EXIT_SUCCESS;
}


/********** Function Definitions **********/
void* get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_in_port(struct sockaddr* sa) {
        if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in*)sa)->sin_port;
    }
    return ((struct sockaddr_in6*)sa)->sin6_port;
}

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

void add_client(client_t* client) {
    pthread_mutex_lock(&clients_mutex);

    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = client;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid) {
    
    pthread_mutex_lock(&clients_mutex);
    int i = 0;

    /* Cycle through MAX_CLIENTS and deleted matching uid */
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void send_message(char* msg, int uid) {
    pthread_mutex_lock(&clients_mutex);

    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (clients[i]->uid != uid) {
                if (write(clients[i]->sockfd, msg, strlen(msg)) < 0) {
                    fprintf(stderr, "Error: unable to send message\n");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg) {
    char buffer[BUFFER_SIZE];
    char name[NAME_SIZE];
    int leave_flag = 0;

    client_count++;
    client_t* client = (client_t*)arg;

    /* Get Name */
    if (recv(client->sockfd, name, NAME_SIZE, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_SIZE - 1) {
        fprintf(stderr, "Error: Name format incorrect\n");
        leave_flag = 1;
    } else {
        strncpy(client->name, name, NAME_SIZE);
        sprintf(buffer, "%s has joined the chatroom\n", client->name);
        fprintf(stdout, "%s", buffer);
        send_message(buffer, client->uid);
    }

    memset(buffer, 0, BUFFER_SIZE);

    while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);

        if (receive > 0) {
            if (strlen(buffer) > 0) {
                send_message(buffer, client->uid);
                str_trim_lf(buffer, strlen(buffer));
                fprintf(stdout, "%s -> %s", buffer, client->name);
            }
        } else if (receive == 0 || strncmp(buffer, "exit", sizeof(buffer) == 0)) {
            sprintf(buffer, "%s has left the chatroom\n", client->name);
            fprintf(stdout, "%s", buffer);
            send_message(buffer, client->uid);
            leave_flag = 1;
        } else {
            fprintf(stderr, "Error: -1\n");
            leave_flag = -1;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(client->sockfd);
    remove_client(client->uid);
    free(client);
    client = NULL;
    client_count--;
    pthread_detach(pthread_self());

    return NULL;
}