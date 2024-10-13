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
//#include <sys/queue.h>
#include "queue.h"

#define PORT "9000" // the port users will be connecting to 
//const char* port = "9000";

//optional
#define BACKLOG 10 // pending connections
#define BUF_SIZE 1024

typedef struct thread_info_s thread_info_t;
typedef struct slist_data_s my_threads;
pthread_mutex_t writeSocket = PTHREAD_MUTEX_INITIALIZER;


struct thread_info_s{
    pthread_t threadid;
    int afd; //accepted fd psot accept() connection.
    bool thread_complete_success;
    char ip4[INET_ADDRSTRLEN]; //space to hold the IPv4 string
};

struct slist_data_s {

    thread_info_t thread_info; 
    SLIST_ENTRY(slist_data_s) nextThread;
};

SLIST_HEAD(slisthead, slist_data_s) head;




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
void *threadfunc(void *arg);

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
    pthread_mutex_destroy(&writeSocket);
    syslog(LOG_DEBUG, "CLEANUP COMPLETED.");
    closelog();
    exit(exit_flag);
}


void *threadfunc(void *args)
{
    my_threads *thread_func_args = (my_threads *) args;
    syslog(LOG_DEBUG, "I have entered this thread with threadID = %lu", thread_func_args->thread_info.threadid);
    bool isPacketValid = false;
    ssize_t bytes_rx;
    ssize_t bytes_read;
    ssize_t supplementBuf = 0;
    ssize_t supplementSize = BUF_SIZE;
    ssize_t nr;
    int rc;
    char *new_line = NULL;
    char *my_buffer = (char *) malloc(BUF_SIZE);
    char *send_my_buffer = NULL;

    if(my_buffer == NULL)
    {
        syslog(LOG_ERR, "Failed to malloc.");
        closeAll(EXIT_FAILURE);
    }

    memset(my_buffer, 0, BUF_SIZE);

    do{
            
            bytes_rx = recv(new_fd, my_buffer+supplementBuf, BUF_SIZE-1, 0); 
            syslog(LOG_DEBUG, "I have received %ld bytes", bytes_rx);
            if (bytes_rx < 0)
            {
                free(my_buffer);
                perror("server: recv");
                syslog(LOG_ERR, "server: recv");
                closeAll(EXIT_FAILURE);
            }
            supplementBuf += bytes_rx; 
            new_line = strchr(my_buffer, '\n'); //if NULL, keep getting packets
            if (new_line != NULL)
            {
                syslog(LOG_DEBUG, "YAY, slash n found");
                isPacketValid = true;
                supplementBuf = new_line - my_buffer + 1; 
                syslog(LOG_DEBUG, "supplementBuf size within slash found %ld", supplementBuf);
            }

            else
            {
                my_buffer = realloc(my_buffer, supplementSize+BUF_SIZE);
                if(my_buffer == NULL)
                {
                    syslog(LOG_ERR, "Failed to realloc.");
                    closeAll(EXIT_FAILURE);
                }
                supplementSize += BUF_SIZE;
                memset(my_buffer+supplementBuf, 0 , supplementSize-supplementBuf);
            }

    }while (isPacketValid == false);



    //new line character has been found
    if(isPacketValid == true)
    {   
        syslog(LOG_DEBUG, "PACKET SUCCESSFULLY VALIDATED");



        rc = pthread_mutex_lock(&writeSocket);
        if (rc != 0)
        {
            syslog(LOG_ERR, "pthread_mutex_lock failed, error was %d", rc);
            perror("pthread mutex_lock failed");
        }
        nr = write(recvfile_fd, my_buffer, supplementBuf);

        rc = pthread_mutex_unlock(&writeSocket);
        if (rc != 0)
        {
            syslog(LOG_ERR, "pthread_mutex_unlock failed, error was %d", rc);
            perror("pthread mutex_unlock failed");
        }

        syslog(LOG_DEBUG, "nr is %ld",nr);
        if (nr != supplementBuf)
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
    my_buffer = NULL;


    send_my_buffer = (char *) malloc(supplementBuf);
    if (send_my_buffer == NULL)
    {
        syslog(LOG_ERR, "Failed to malloc sending buffer.");
        closeAll(EXIT_FAILURE);
    }

   

    rc = pthread_mutex_lock(&writeSocket);
    if (rc != 0)
    {
        syslog(LOG_ERR, "pthread_mutex_lock failed, error was %d", rc);
        perror("pthread mutex_lock failed");
    }

    if(lseek(recvfile_fd, 0, SEEK_SET) == -1)
    {
        syslog(LOG_ERR, "server: lseek");
        perror("server: lseek");
        closeAll(EXIT_FAILURE);
    }
    syslog(LOG_DEBUG, "lseek pass");


    while ((bytes_read = read(recvfile_fd, send_my_buffer, supplementBuf)) > 0) {
        syslog(LOG_DEBUG, "bytes_read is %ld", bytes_read);
        if (send(new_fd, send_my_buffer, bytes_read, 0) != bytes_read)
        {
            syslog(LOG_DEBUG,"HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
            syslog(LOG_ERR,"server: send");
            rc = pthread_mutex_unlock(&writeSocket);
            if (rc != 0)
            {
                syslog(LOG_ERR, "pthread_mutex_unlock failed, error was %d", rc);
                perror("pthread mutex_unlock failed");
            }
            perror("server: send");
            closeAll(EXIT_FAILURE);
        }
        else
        {
            syslog(LOG_DEBUG, "send passed");
            free(send_my_buffer);
            send_my_buffer = NULL;
        }
    }

    rc = pthread_mutex_unlock(&writeSocket);
    if (rc != 0)
    {
        syslog(LOG_ERR, "pthread_mutex_unlock failed, error was %d", rc);
        perror("pthread mutex_unlock failed");
    }

    syslog(LOG_DEBUG, "_________________________________________________________");
    thread_func_args->thread_info.thread_complete_success = true;
    close(new_fd);
    syslog(LOG_DEBUG, "About to exit thread ID = %lu", thread_func_args->thread_info.threadid);
    pthread_exit(NULL);
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
    SLIST_INIT(&head);
    //pthread_mutex_init(&writeSocket, NULL);

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

    
    
    while(!sig){ 
        
        peer_addrlen = sizeof(peer_addr);
        if((new_fd = accept(sockfd, (struct sockaddr *)&peer_addr, &peer_addrlen)) == -1 )
        {
            syslog(LOG_ERR, "server: accept");
            perror("server: accept");
            continue;
        }
        
        syslog(LOG_DEBUG, "CONNECTED, new_fd is %d", new_fd);

        inet_ntop(AF_INET, &(peer_addr.sin_addr), ip4, sizeof(ip4));
        syslog(LOG_DEBUG, "Accepted connection from %s", ip4);

        my_threads *thread_data = (my_threads *)malloc(sizeof(my_threads));
        if (thread_data == NULL)
        {
            syslog(LOG_ERR, "server: thread allocation");
            perror("server: thread allocation");
            continue;
            //should I be exiting everything here or just keep trying
        }
        
        

        thread_data->thread_info.afd = new_fd;
        thread_data->thread_info.thread_complete_success = false;
        strcpy(thread_data->thread_info.ip4, ip4);

        int rc;

        if((rc = pthread_create(&thread_data->thread_info.threadid, NULL, threadfunc, (void*) &(thread_data->thread_info))) != 0)
        {
            syslog(LOG_ERR, "Failed to pthread_create(), error was %d", rc);
            perror("Failed to pthread_create()");
            free(thread_data);
            close(new_fd);
            continue;
        }

        else 
        {
            syslog(LOG_DEBUG, "pthread_created successfully");
            syslog(LOG_DEBUG, "The threadID is %lu.", thread_data->thread_info.threadid);
        }

        SLIST_INSERT_HEAD(&head, thread_data, nextThread);
        int join_rc;
        my_threads *datap = NULL;
        my_threads *temp = NULL;

        SLIST_FOREACH_SAFE(datap, &head, nextThread, temp)
        {
            if(datap->thread_info.thread_complete_success)
            {
                join_rc = pthread_join(datap->thread_info.threadid, NULL);
                if(join_rc != 0) 
                {
                    syslog(LOG_ERR, "Failed to pthread_join(), error was %d", join_rc);
                    perror("Failed to pthread_join()");
                    continue;
                }
                syslog(LOG_DEBUG, "Joined thread %lu", datap->thread_info.threadid);
                SLIST_REMOVE(&head, datap, slist_data_s, nextThread);
                free(datap);
                datap = NULL;
            }
            else
            {
                syslog(LOG_DEBUG, "I did not join on threadID = %lu", datap->thread_info.threadid);
            }
        }
    }

    syslog(LOG_DEBUG, "Do I reach here?");
    closeAll(EXIT_SUCCESS);
    return 0;
}