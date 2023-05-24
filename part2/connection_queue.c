#include <stdio.h>
#include <string.h>
#include "connection_queue.h"

int connection_queue_init(connection_queue_t *queue) {
    memset(queue->client_fds, 0, sizeof(queue->client_fds));
    //start with all zeroes
    queue->length = 0;
    queue->shutdown=0;
    int result;
    if ((result = pthread_mutex_init(&queue->lock, NULL)) != 0) {
        //initialize the lock
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_init(&queue->queue_full, NULL)) != 0) {
        //initialize condition
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_init(&queue->queue_empty, NULL)) != 0) {
        //initialize condition
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(result));
        return -1;
    }
    return 0;
}

int connection_enqueue(connection_queue_t *queue, int connection_fd) {
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        //Lock while modifying
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    while (queue->length == CAPACITY && queue->shutdown!=1) {
        //until shutdown is called, if the queue is full, then wait until one is dequeued
        if ((result = pthread_cond_wait(&queue->queue_full, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            return -1;
        }
    }
    if(queue->shutdown==1){
        //shutdown, ends command and returns immediately
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    //add the new fd to the queue at index = length
    queue->client_fds[queue->length] = connection_fd;
    queue->length++;
    //increase length by 1

    if ((result = pthread_cond_signal(&queue->queue_empty)) != 0) {
        //signal that it is not empty, allowing dequeues to occur.
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        //unlock and return as successful
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return 0;
}

int connection_dequeue(connection_queue_t *queue) {
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        //lock for the duration of the command
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    while (queue->length == 0 && queue->shutdown!=1) {
        //until shutdown is called, if the queue is empty, then wait until one is enqueued
        if ((result = pthread_cond_wait(&queue->queue_empty, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
    }
    if(queue->shutdown==1){
        //if shutdown is called immediately return.
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    queue->length--;
    //remove one from the length of the list
    
    int returnval =  queue->client_fds[0];
    //save the client_fd that is needed
	for(int i=1;i<CAPACITY-1;i++) {
        //moving every client_fds one to the left.
        //I tried this with bitwise operations but this was easier.
		queue->client_fds[i-1]= queue->client_fds[i]; 
	}
    if ((result = pthread_cond_signal(&queue->queue_full)) != 0) {
        //allow anyone to enqueue if it was full, indicating there is room
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        //unlock at the end and return.
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return returnval;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    int result;
    queue->shutdown = 1;
    //update shutdown to indicate the system is over.
   if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        //lock at the start to prevent memory issues
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    if(pthread_cond_broadcast(&queue->queue_full)!=0){
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    //broadcast the signal to all threads, allowing them to finish their wait, and immediately terminate.
    if(pthread_cond_broadcast(&queue->queue_empty)!=0){
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        //unlock at the end and return.
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    int result;
    if ((result = pthread_mutex_destroy(&queue->lock)) != 0) {
        //unlock at the end and return.
        fprintf(stderr, "pthread_mutex_destroyed: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_destroy(&queue->queue_full)) != 0) {
        //unlock at the end and return.
        fprintf(stderr, "pthread_cond_destroyed: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_destroy(&queue->queue_empty)) != 0) {
        //unlock at the end and return.
        fprintf(stderr, "pthread_cond_destroyed: %s\n", strerror(result));
        return -1;
    }
    return 0;
}
