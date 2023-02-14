#ifndef _NETWORK_UTIL_H_
#define _NETWORK_UTIL_H_
/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: February 12, 2023
 * Desc: Network Utility Functions
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

/*
 * Author: Chris Martinez
 * Version: 1.0
 * Date: January 23, 2023
 * Desc: client_t struct
*/
typedef struct client_t{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[INET6_ADDRSTRLEN];
} client_t;

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

#endif