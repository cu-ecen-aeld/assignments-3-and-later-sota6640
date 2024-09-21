/**
 * @file    threading.c
 * @brief	Assignment 4 Part 1
 * @editor  Sonal Tamrakar
 * @date    09-22-2024
 * @credit  Chapter 7: Threading Linux System Programming, Robert Love
 */
#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>


#define USDELAYCONV     (1000)
// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    int rc;

    // Obtaining the thread arguments from the passed in thread_param which is a void*
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Microsleep function to wait to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms*USDELAYCONV);

    // Lock the mutex
    rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0)
    {
        ERROR_LOG("pthread_mutex_lock failed, error was %d", rc);
        return thread_param;
    }
    // Microsleep function to wait to release the mutex
    usleep(thread_func_args->wait_to_release_ms*USDELAYCONV);

    // Unlock the mutex
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0)
    {
        ERROR_LOG("pthread_mutex_unclock failed, error was %d", rc);
        return thread_param;
    }

    // Thread was completed with success
    thread_func_args->thread_complete_success = true;

    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{   
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     **/

    int rc;

    struct thread_data* threadp = (struct thread_data*)malloc(sizeof(struct thread_data));
    if (threadp == NULL)
    {
        ERROR_LOG("Failed to allocate memory for thread data");
        return false;
    }

    threadp->thread_complete_success = false;
    threadp->mutex = mutex;
    threadp->wait_to_obtain_ms = wait_to_obtain_ms;
    threadp->wait_to_release_ms = wait_to_release_ms;


    rc = pthread_create(thread, NULL, threadfunc, (void*)threadp);
    if (rc != 0)
    {
        ERROR_LOG("Failed to pthread_create(), error was %d", rc);
        free(threadp);
        return false;
    }

    DEBUG_LOG("Successful in pthread_create()");

    return true;;
}

