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
 * @file    writer.c
 * @brief	Assignment 1/2. C implementation of writer.sh from Assignment 1
 * @author  Sonal Tamrakar
 * @date    09-08-2024
 * @credit  Chapter 2: File I/O Linux System Programming, Robert Love
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
    // Setting up syslog logging using LOG_USER facility
    openlog("writerClogging", LOG_CONS | LOG_PID , LOG_USER);
    // No arguments were provided or partial arguments
    if(argc < 3) {
        syslog(LOG_ERR, "Failed to open file, arguments should be three");
        exit(1);
    }

    // Arguments given weren't 3. Show debug information
    if(argc != 3) {
        syslog(LOG_ERR, "Arguments inconsistent. Expected order of the arguments should be:");
        syslog(LOG_INFO, "1) Name of the program being executed.");
        syslog(LOG_INFO, "2) File Directory Path Including Filename.");
        syslog(LOG_INFO, "3) String WRITESTR to be written in the specified directory file.");
        exit(1);
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];
    int fd;
    ssize_t nr;

    /*
    O_WRONLY: only for writing
    O_CREAT: kernel creates file if it doesn't exist
    O_TRUNC: overwrites everything in the file, makes it zero length
    Permissions: owner + group has read/write permissions, everyone
    else can just read
    */
    fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC,
            S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd == -1) 
    {
        /*error*/
        int err = errno;
        syslog(LOG_ERR, "%s failed to open. errno -> %d", writefile, err);
        exit(1);
    }

    nr = write(fd, writestr, strlen(writestr));


    if (nr != strlen(writestr))
    {
        /*error*/
        int err1 = errno;
        syslog(LOG_ERR, "Failed to write %s to %s. errno -> %d", writestr, writefile, err1);
        exit(1);
    }
    
    //exact match
    else if (nr == strlen(writestr))
    {
        syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    }

    close(fd);
    closelog();
    return 0;
}