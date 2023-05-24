#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "http.h"
#define BUFSIZE 512
#define FILESIZE 4096
const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    char fullrequest[FILESIZE];
    int found=0;
    memset(fullrequest,0,FILESIZE);
    read(fd, fullrequest, FILESIZE);//Was told to assume file is less than 4096 bytes
    //read all of the file
        if(found == 0){
            sscanf(fullrequest, "GET %s \r\n", resource_name);//look for the starting request for GET
            //store resource name
            found = 1;
        }
    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    struct stat m;
    int statval = stat(resource_path, &m);
    //stat to determine size of file and also if it exists
    if(statval==-1){
        if(errno == ENOENT){
            //file not found  = error 404
            write(fd, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n", sizeof("HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n"));
            return 0;
        }
        else{ //else other error means error out
            perror("stat");
            return -1;
        }
    }
    char buf[BUFSIZE];//buffer to write to file
    memset(buf, 0, BUFSIZE);
    //this is to format the header, inserting size and the correct filetype.
    //strrchr(resource_path, '.') gives us the pointer to after the ., which is the end of our string.
    sprintf(buf, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", get_mime_type(strrchr(resource_path, '.')), m.st_size);
    //write the header to client
    if(write(fd, buf, strlen(buf))==-1){
        perror("write");
        return -1;
    }
    int n;
    int filefd = open(resource_path, O_RDONLY);
    //open the requested file
    while((n=read(filefd, buf, BUFSIZE))>0){
        //write from given file to client
        write(fd,buf,n);
    }
    if(n==-1){
        //if there is an error, return -1
        perror("write");
        close(filefd);
        return -1;
    }
    close(filefd);
    return 0;
}
