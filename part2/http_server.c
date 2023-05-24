#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define FILESIZE 4096
#define NTHREADS 5
int keep_going = 1;

typedef struct {
   connection_queue_t *queue;
   const char *serve_dir;
} thread_args_t;

void handle_sigint(int signo) {
    keep_going = 0;
}
void *thread_func(void *arg) {
    thread_args_t *threadargs = (thread_args_t *) arg;
    //turn void arg pointer into a threadargs struct
    while(keep_going){
        int client_fd = connection_dequeue(threadargs->queue);//take client_fd from the queue
        if(client_fd==-1){
            continue;
        }
        char buf[BUFSIZE]; //returns with filename to GET
        char buf2[BUFSIZE]; //gives Filename to writehttprequest
        memset(buf, 0, BUFSIZE);
        read_http_request(client_fd, buf);//read what the request wants
        memset(buf2,0,BUFSIZE);
        strcat(buf2, threadargs->serve_dir);
        //strcat(buf2, "/");
        strcat(buf2, buf);
        write_http_response(client_fd, buf2);//write response to client
        //error messages come from http.c
        close(client_fd);//close client, before loop reiterates
    }
    return NULL;
}
int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    struct sigaction act;//Signal handler on SIGINT
    act.sa_handler  = handle_sigint;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    // Uncomment the lines below to use these definitions:
    connection_queue_t cqueue;//Set up ConnectionQ
    if(connection_queue_init(&cqueue)!=0){
        printf("connection_queue_init, failed to initialize");
        return 1;
    }
    const char *serve_dir = argv[1];//Arguments from command line
    const char *port = argv[2];
    struct addrinfo hints; //hints and server setup
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server;
    int erval = getaddrinfo(NULL, port, &hints, &server);//set up server
    if(erval != 0){
        printf("%s",gai_strerror(erval));
        return 1;
    }
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    //create a socket
    if (sock_fd == -1) {
        perror("Socket");
        freeaddrinfo(server);
        return 1;
    }
    //bind to socket
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }
    //listen
    if(listen(sock_fd, LISTEN_QUEUE_LEN)<0){
        perror("listen");
        close(sock_fd);
        freeaddrinfo(server);
        return -1;
    }
    sigset_t all, previous;
    sigfillset(&all);
    if(sigprocmask(SIG_BLOCK, &all, &previous)==-1){//Block all signals in the threads
        perror("sigprocmask");
        close(sock_fd);
        freeaddrinfo(server);
        return -1;
    }
    pthread_t mythreads[NTHREADS];
    thread_args_t threadargs;
    threadargs.queue = &cqueue;
    threadargs.serve_dir = serve_dir;
    int result;
    for(int i=0;i<NTHREADS;i++){//Create NTHREAD threads, each running my thread_func command.
        if((result = pthread_create(mythreads + i, NULL, thread_func, &threadargs)) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            return 1;
        }
    }
    freeaddrinfo(server);//This could be earlier but it works here
    if(sigprocmask(SIG_SETMASK, &previous, NULL)==-1){//reset the signal mask to what it was before the thread loop. 
        perror("sigprocmask");
        close(sock_fd);
        freeaddrinfo(server);
        return -1;
    }
    while(keep_going==1) {
        //accept waits until a request is made.
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {//if interrupt we need to end the loop and close as usual, otherwise error and report.
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                freeaddrinfo(server);
                return 1;
            } else {
                break;
            }
        }
        if(connection_enqueue(&cqueue, client_fd)==-1){//add the file descriptor to the queue. 
            perror("accept");
            close(sock_fd);
            freeaddrinfo(server);
            close(client_fd);
            return 1;
        }
    }
    close(sock_fd);//Now we close shutdown and destroy as part of our cleanup
    int exit_code =0;
    connection_queue_shutdown(&cqueue);
    for (int i = 0; i < NTHREADS; i++) {//for each thread with join. 
        if ((result = pthread_join(mythreads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            exit_code = -1;
        }
    }
    connection_queue_free(&cqueue);//final cleanup step.
    return exit_code;//exit code is not necessary, but if this were run as part of a larger application could be useful to know why the cancellation occurred.
}
