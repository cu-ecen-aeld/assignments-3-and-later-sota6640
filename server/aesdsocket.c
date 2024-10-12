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
 * @credit4 Linux System Programming Chapter 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <linux/fs.h>
#include <pthread.h>
#include <sys/queue.h>

#define PORT "9000" // the port users will be connecting to 
//const char* port = "9000";

//optional
#define BACKLOG 10 // pending connections
#define BUF_SIZE 1024


typedef struct slist_data_s my_threads;

struct slist_data_s {
    bool thread_complete_success;
    SLIST_ENTRY(slist_data_s) threads;
};

sig_atomic_t sig = 0;
bool daemon_en = false;

int sockfd = -1;
int new_fd = -1;
int recvfile_fd = -1;

pid_t pid;
const char *recvfile = "/var/tmp/aesdsocketdata";
static void closeAll(int exit_flag);
static void init_sigHandler(void);
static void signal_handler(int signal_number);

static void signal_handler (int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        sig = 1;
    }
}


static void init_sigHandler(void)
{
    struct sigaction sig1;
    memset(&sig1, 0, sizeof(struct sigaction));
    sig1.sa_handler = signal_handler;
    if( sigaction(SIGINT, &sig1, NULL) != 0)
    {
        syslog(LOG_ERR, "Error %d (%s) registering for SIGINT", errno, strerror(errno));
        perror("sigint: fail");
        closeAll(EXIT_FAILURE);
    }

    struct sigaction sig2;
    memset(&sig2, 0, sizeof(struct sigaction));
    sig2.sa_handler = signal_handler;
    if( sigaction(SIGTERM, &sig2, NULL) != 0)
    {
        syslog(LOG_ERR, "Error %d (%s) registering for SIGTERM", errno, strerror(errno));
        perror("sigterm: fail");
        closeAll(EXIT_FAILURE);
    }

    return;
}


static int create_daemon()
{
        pid = fork();
        if (pid == -1)
        {
            syslog(LOG_ERR, "fork()");
            perror("fork");
            return -1;
        }

        if (pid != 0)
        {
            //parent exits
            exit(EXIT_SUCCESS);
        }

        /* create new session and process group */
        if (setsid() == -1) {
            syslog(LOG_ERR, "setsid()");
            perror("setsid()");
            return -1;
        }

        /* set the working directory to the root directory */
        if (chdir ("/") == -1) {
            syslog(LOG_ERR, "chdir()");
            perror("chdir");
            return -1;
        }

        /* Close file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        /* Redirect stdin, stdout, stderr to /dev/null */
        open ("/dev/null", O_RDWR); // stdin
        dup(0); // stdout
        dup(0); // stderror

        return 0;
}


void closeAll(int exit_flag)
{
    syslog(LOG_DEBUG, "PERFORMING CLEANUP.");
    if(sockfd != -1) close(sockfd);
    if(new_fd != -1) close(new_fd);
    if(recvfile_fd != -1) close(recvfile_fd);
    if(remove(recvfile) != 0)
    {
        syslog(LOG_ERR, "File removal not successful");
    }
    syslog(LOG_DEBUG, "CLEANUP COMPLETED.");
    closelog();
    exit(exit_flag);
}

int main(int argc, char *argv[])
{
    // Setting up syslog logging using LOG_USER facility
    openlog("aesdsocketLogging", LOG_CONS | LOG_PID , LOG_USER);

    syslog(LOG_DEBUG, "__________________________./full-test.sh________________________");

    int                             status;
    socklen_t                       peer_addrlen;
    struct addrinfo                 hints;
    struct addrinfo                 *servinfo;
    struct sockaddr_in              peer_addr;
    char ip4[INET_ADDRSTRLEN]; //space to hold the IPv4 string
    int yes=1;


    //Initializes the signal handlers SIGINT and SIGTERM
    init_sigHandler();
    

    if ((argc == 2) && (strcmp(argv[1], "-d") == 0))
        daemon_en = true;
    else    
        daemon_en = false;


    //Receives data over the connection and appends to file "/var/tmp/aesdsocketdata", creating this file if it doesn't exist.
    recvfile_fd = open(recvfile, O_RDWR | O_CREAT | O_TRUNC,
            S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (recvfile_fd == -1) 
    {
        /*error*/
        int err = errno;
        syslog(LOG_ERR, "%s failed to open. errno -> %d", recvfile, err);
        closeAll(EXIT_FAILURE);
    }


    memset(&hints, 0, sizeof(hints)); //empty the struct
    hints.ai_family = AF_UNSPEC;      //Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;   // Datagram socket
    hints.ai_flags = AI_PASSIVE;      // For wildcard IP address, fill in my IP for me


    // &servInfo is the not the pointer to the addrinfo struct,
    // but rather pointer to the location to store the pointer to that addrinfo
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        closeAll(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "getaddrinfo pass");


    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        perror("server: socket");
        closeAll(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "socket pass");
 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("server: setsockopt");
        closeAll(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "setsockopt pass");

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        perror("server: bind");
        closeAll(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "bind pass");

    freeaddrinfo(servinfo);

    if(daemon_en)
    {
        if(create_daemon() != 0)
        {
            syslog(LOG_ERR, "serve: daemon");
            perror("server: daemon");
            closeAll(EXIT_FAILURE);
        }
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("server: listen");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "listen pass");

    syslog(LOG_DEBUG, "server: waiting for connections.........\n");

    
    
    ssize_t incrementBy;
    ssize_t bytes_rx = 0;
    ssize_t addibuffer;
    ssize_t bytes_read;
    char *send_my_buffer = NULL;
    char *new_line = NULL;
    ssize_t nr;
    bool isPacketValid = false;

    while(!sig){ 
        bytes_read = 0;
        incrementBy = BUF_SIZE;
        isPacketValid = false;
        peer_addrlen = sizeof(peer_addr);
        if((new_fd = accept(sockfd, (struct sockaddr *)&peer_addr, &peer_addrlen)) == -1 )
        {
            syslog(LOG_ERR, "server: accept");
            perror("server: accept");
            continue;
        }
        
        syslog(LOG_DEBUG, "CONNECTED");
        
        // int rc;
        // rc = pthread_create(threadid, NULL, threadfunc, (void*)new_fd);
        // if (rc != 0)
        // {   
        //     ERROR_LOG("Failed to pthread_create(), error was %d", rc);
        //     free(threadp);
        //     return false;
        // }


        //create thread here ?

        inet_ntop(AF_INET, &(peer_addr.sin_addr), ip4, sizeof(ip4));
        syslog(LOG_DEBUG, "Accepted connection from %s", ip4);

        addibuffer = 0;
        char *my_buffer = (char *) malloc(BUF_SIZE);
        if (my_buffer == NULL)
        {
            syslog(LOG_ERR, "Failed to malloc.");
            closeAll(EXIT_FAILURE);
        }
        memset(my_buffer, 0, BUF_SIZE);
        do{
            
            bytes_rx = recv(new_fd, my_buffer + addibuffer, BUF_SIZE-1, 0); 
            syslog(LOG_DEBUG, "i have received %ld bytes", bytes_rx);
            if (bytes_rx < 0)
            {
                free(my_buffer);
                perror("server: recv");
                syslog(LOG_ERR, "server: recv");
                closeAll(EXIT_FAILURE);
            }
            addibuffer += bytes_rx; 
            new_line = strchr(my_buffer, '\n'); //if NULL, keep getting packets
            if (new_line != NULL)
            {
                syslog(LOG_DEBUG, "YAY, slash n found");
                isPacketValid = true;
                addibuffer = new_line - my_buffer + 1; 
                syslog(LOG_DEBUG, "addibuffer size within slash found %ld", addibuffer);
            }

            else
            {
                my_buffer = realloc(my_buffer, incrementBy+BUF_SIZE);
                if(my_buffer == NULL)
                {
                    syslog(LOG_ERR, "Failed to realloc.");
                    closeAll(EXIT_FAILURE);
                }
                incrementBy += BUF_SIZE;
                memset(my_buffer+addibuffer, 0 , incrementBy-addibuffer);
            }

        }while (isPacketValid == false);

        //new line character has been found, now i want to send?
        //append happens here, writing
        if(isPacketValid == true)
        {   
            //my_buffer[addibuffer]= '\0';
            //ssize_t write_size = new_line - my_buffer + 1;
            syslog(LOG_DEBUG, "PACKET SUCCESSFULLY VALIDATED");
            nr = write(recvfile_fd, my_buffer, addibuffer);
            syslog(LOG_DEBUG, "nr is %ld",nr);
            if (nr != addibuffer)
            {
                /*error*/
                int err1 = errno;
                syslog(LOG_ERR, "Failed to write bytes: errno -> %d", err1);
                syslog(LOG_ERR, "error string is %s", strerror(errno));
                closeAll(EXIT_FAILURE);
            }

            syslog(LOG_DEBUG, "Write completed to recvfile_fd");
        }


        free(my_buffer);

        if(lseek(recvfile_fd, 0, SEEK_SET) == -1)
        {
            syslog(LOG_ERR, "server: lseek");
            perror("server: lseek");
            closeAll(EXIT_FAILURE);
        }

        syslog(LOG_DEBUG, "lseek pass");
        //syslog(LOG_DEBUG, "addibuffer is %ld", addibuffer);
        send_my_buffer = (char *) malloc(addibuffer);
        if (send_my_buffer == NULL)
        {
            syslog(LOG_ERR, "Failed to malloc sending buffer.");
            closeAll(EXIT_FAILURE);
        }

        
        //int errno_read; 
        //syslog(LOG_DEBUG, "I am here. Just before the while. addibuffer is %ld, %d", addibuffer, recvfile_fd);
        while ((bytes_read = read(recvfile_fd, send_my_buffer, addibuffer)) > 0) {
            syslog(LOG_DEBUG, "bytes_read is %ld", bytes_read);
            //syslog(LOG_DEBUG, "bytes_read inside while is %ld", bytes_read);
            // for( int i = 0; i < bytes_read; i++)
            // {
            //     syslog(LOG_DEBUG, "%c", send_my_buffer[i]);
            // }

            //syslog(LOG_DEBUG, "strlen of send_my_buffer is %ld", strlen(send_my_buffer));
            //syslog(LOG_DEBUG, "new_fd heree is %d", new_fd);
            if (send(new_fd, send_my_buffer, bytes_read, 0) != bytes_read)
            {
                syslog(LOG_DEBUG,"HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
                syslog(LOG_ERR,"server: send");
                perror("server: send");
                closeAll(EXIT_FAILURE);
            }
            else
            {
                syslog(LOG_DEBUG, "send pass");
            }
        }

        close(new_fd);
        free(send_my_buffer);
        syslog(LOG_DEBUG, "Closed connection from %s", ip4);
        syslog(LOG_DEBUG, "_________________________________________________________");
    }

    syslog(LOG_DEBUG, "AM I EVER HERE?");
    closeAll(EXIT_SUCCESS);
}