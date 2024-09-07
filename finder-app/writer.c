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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


int main(int argc, char *argv[])
{
    openlog("writerClogging", LOG_CONS | LOG_PID , LOG_USER);
    // No arguments were provided. 
    if(argc < 3) {
        syslog(LOG_ERR, "Failed to open file, arguments should be three");
    }

    // Arguments given weren't 3. 
    if(argc != 3) {
        syslog(LOG_ERR, "Arguments inconsistent. Expected order of the arguments should be:");
        syslog(LOG_INFO, "1) Name of the program being executed.");
        syslog(LOG_INFO, "2) File Directory Path Including Filename.");
        syslog(LOG_INFO, "3) String WRITESTR to be written in the specified directory file.");
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];
    int fd;
    ssize_t nr;

    fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC,
            S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd == -1) 
    {
        /*error*/
        int err = errno;
        syslog(LOG_ERR, "%s failed to open. errno -> %d", writefile, err);
    }

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    
    nr = write(fd, writestr, strlen(writestr));
    if (nr == -1)
    {
        /*error*/
        int err1 = errno;
        syslog(LOG_ERR, "Failed to write %s to %s. errno -> %d", writestr, writefile, err1);
    }

    close(fd);
    closelog();
    return 0;
}