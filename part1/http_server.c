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
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define FILESIZE 4096

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    struct sigaction act;//Signal handler on SIGINT
    act.sa_handler  = handle_sigint;
    act.sa_flags = SA_RESTART;
    sigaction(SIGINT, &act, NULL);
    // Uncomment the lines below to use these definitions:
    const char *serve_dir = argv[1];
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
    freeaddrinfo(server);
    //bind to socket
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("bind");
        close(sock_fd);
        return 1;
    }
    //listen
    if(listen(sock_fd, LISTEN_QUEUE_LEN)<0){
        perror("listen");
        close(sock_fd);
        exit(1);
    }
    //was told by TA to assume that input is smaller than bufsize
    char buf[BUFSIZE]; //returns with filename to GET
    char buf2[BUFSIZE]; //gives Filename to writehttprequest
    while(keep_going==1) {
        //Wait until we receive a request to the server
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                freeaddrinfo(server);
                return 1;
            } else {//interrupt just ends the whole loop
                break;
            }
        }
        memset(buf, 0, BUFSIZE);
        read_http_request(client_fd, buf);//read what the request wants
        memset(buf2,0,BUFSIZE);
        strcat(buf2, serve_dir);//does not set errval, so I cannot errchech
        //strcat(buf2, "/");
        strcat(buf2, buf);
        write_http_response(client_fd, buf2);//write response to client
        close(client_fd);//close client, before loop reiterates
    }
    close(sock_fd);//doesn't matter if it errors out, since we are freeing and exiting on interrupt anyway
    return 0;
}
