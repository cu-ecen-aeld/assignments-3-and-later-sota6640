#include "systemcalls.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
/**
 * @file    systemcalls.c
 * @brief	Assignment 3. exec() + system() function implementations
 * @editor  Sonal Tamrakar
 * @date    09-09-2024
 * @credit  Chapter 5: Process Management Linux System Programming, Robert Love
 * @credit2 https://stackoverflow.com/questions/13784269/redirection-inside-call-to-execvp-not-working/13784315#13784315
 */



/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int ret;
    if (cmd == NULL)
    {
        ret = system(cmd);
        if(!ret) return false; //no shell is available
        //shell is available
    }
    
    ret = system(cmd);

    //a child process couldn't be created or status couldn't be received
    if (ret != 0) return false;

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    pid_t pid;
    int status;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return false;
    }

    /* the child ... */
    if (!pid)
    {
        int execv_stat;
        execv_stat = execv(command[0], command);
        if (execv_stat == -1)
        {
            perror("execv");
            printf("execv fail do_exec()\n");
            exit(EXIT_FAILURE);
        }
    }

    if (pid > 0)
    {
        pid_t stat;
        stat = waitpid(pid, &status,0); //WNOHANG OR 0?
        if (stat == -1) 
        {
            perror("waitpid");
            return false;
        }
        
        //Normal termination
        if(WIFEXITED(status) != 0)
        {
            if(WEXITSTATUS(status) != 0) return false;
        }
        else return false;

    }

    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    pid_t pid;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 
                S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
                    
    if (fd == -1) 
    {
        /*error opening file*/
        perror("open");
        return false;
    }

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return false;
    }

    else if (pid == 0)
    {
        if (dup2(fd,1) < 0)
        {
            perror("dup2");
            close(fd);
            return false;
        }
        int execv_stat = execv(command[0], command);
        if (execv_stat == -1)
        {
            perror("execv");
            printf("execv fail do_exect_redirect()\n");
            exit(EXIT_FAILURE);
        }
    }
    
    else
    {
        close(fd);
    }


    if (pid > 0)
    {
        pid_t stat;
        int status;
        stat = waitpid(pid, &status,0); //WNOHANG OR 0?
        if (stat == -1) 
        {
            perror("waitpid");
            return false;
        }
        
        //Normal termination
        if(WIFEXITED(status) != 0)
        {
            if(WEXITSTATUS(status) != 0) return false;
        }
        else return false;

    }

    va_end(args);

    return true;
}
