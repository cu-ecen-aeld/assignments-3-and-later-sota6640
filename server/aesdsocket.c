/*******************************************************************************
 * Copyright (C) 2024 by Sonal Tamrakar
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Sonal Tamrakar and the University of Colorado are not liable for
 * any misuse of this material.
 * ****************************************************************************/
/**
 * @file    aesdsocket.c
 * @brief	Assignment 5 Part 1. Socket based program
 * @author  Sonal Tamrakar
 * @date    10-07-2024
 * @credit  Beej's Guide to Network Programming
 * @credit2 Video Content ECEN 5713 Fall 2024
 * @credit3 https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
 */

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

#define PORT "9000" // the port users will be connecting to 
//const char* port = "9000";

//optional
#define BACKLOG 10 // pending connections
#define BUF_SIZE 500


int main(void)
{
    int                             status;
    int                             sockfd, new_fd;
    char                            buf[BUF_SIZE];
    ssize_t                         nread;
    socklen_t                       peer_addrlen;
    struct addrinfo                 hints;
    struct addrinfo                 *servinfo, *rp;
    struct sockaddr_storage         peer_addr;
    int yes=1;
    /*
    *servInfo is just a pointer location  on the stack. Will point to the results. 
    Haven't reserved location for the actual addrinfo structure
    
    
    */


    memset(&hints, 0, sizeof(hints)); //empty the struct
    hints.ai_family = AF_UNSPEC;      //Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;   // Datagram socket
    hints.ai_flags = AI_PASSIVE;      // For wildcard IP address, fill in my IP for me

    //Not sure if these need to be initialized
    hints.ai_protocol = 0;            // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;


    // &servInfo is the not the pointer to the addrinfo struct,
    // but rather pointer to the location to store the pointer to that addrinfo
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures. Try each address
        until we successfully bind(2)
        */

    // servInfo now points to a linked list of 1 or more struct addrinfos
    //loop through all the results and bind to the first we can
    for (rp = servinfo; rp != NULL; rp = rp->ai_next)
    {
        if ((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        //assign an address to the socket
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (rp == NULL) // No address succeeded
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen");
        exit(1);
    }





    //call bind?
    // int bind(sockfd, const struct sockaddr *addr, socklen_t addrlen);
    // sockaddr addr/addrlen describes the address to bind (optionally select a specific network adapter)
    //sockaddr - maps to a server socket location
    // sa_family - AF_INET or AF_INET6
    // sa_data - destination address and port (9000)
    // socketlen_t - unsigned integer type sizeof(struct sockaddr)
    //call listen?
    //accept?

    //TODO: Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail
    //TODO: Listens for and accepts a connection
    //TODO: Logs message to the syslog "Accepted connected from xxx" where XXXX is the IP address of the connected client.


    return 0;
}