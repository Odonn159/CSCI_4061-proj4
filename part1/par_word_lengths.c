#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_WORD_LEN 25


/*
 * Counts the number of occurrences of words of different lengths in a text
 * file and stores the results in an array.
 * file_name: The name of the text file from which to read words
 * counts: An array of integers storing the number of words of each possible
 *     length.  counts[0] is the number of 1-character words, counts [1] is the
 *     number of 2-character words, and so on.
 * Returns 0 on success or -1 on error.
 */

int count_word_lengths(const char *file_name, int *counts) {
    FILE *fd = fopen(file_name, "r");
    //open for reading
    if(fd==NULL){
        perror("fopen");
        return -1;
    }
    memset(counts, 0, MAX_WORD_LEN* sizeof(int));
    //set counts to 0, make sure it works
    char word[MAX_WORD_LEN];
    int length;
    while(fscanf(fd, "%s", word)==1){
        //scan in the word and continue scanning until you do not find a string, separated by whitespace %s
        length =strlen(word);
        //Store length
        if(length!=0){//This should always occur unless multiple whitespace or null values are next to eachother.
            //add to count
            counts[length-1]+=1;
        }
        memset(word, 0,  MAX_WORD_LEN);
        //Memset may be unecessary, as 
    }
    if(ferror(fd)!=0){
        perror("fscan");
        return -1;
    }
    fclose(fd);
    return 0;
}

/*
 * Processes a particular file (counting the number of words of each length)
 * and writes the results to a file descriptor.
 * This function should be called in child processes.
 * file_name: The name of the file to analyze.
 * out_fd: The file descriptor to which results are written
 * Returns 0 on success or -1 on error
 */
int process_file(const char *file_name, int out_fd) {
    int counts[MAX_WORD_LEN];
    //Initialize 25 integer array
    if(count_word_lengths(file_name, counts)==-1){
        //error message is in count word lengths
        return -1;
    }
    if(write(out_fd, counts, sizeof(int)*MAX_WORD_LEN)!=sizeof(int)*MAX_WORD_LEN){
        //This should always write 100 bits, regardless of the contents. If it fails to it will error.
        perror("process_file");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No files to consume, return immediately
        return 0;
    }
    int pipe_fds[2];
    if(pipe(pipe_fds)==-1){
        perror("pipe");
        return -1;
    }
    // TODO Fork a child to analyze each specified file (names are argv[1], argv[2], ...)
    for(int i=1;i<argc;i++){
        int pid =fork();
        if(pid==-1){
            //only will trigger if fork errors in parent process, child will not exist
            perror("fork");
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            return -1;
        }
        //fork each time
        if(pid==0){
            //In child run process file to store wordcount
            close(pipe_fds[0]);
            //Immediately close read end, as children (processes) do not read
            if(process_file(argv[i],pipe_fds[1])==-1){
                //error message is in process file
                close(pipe_fds[1]);
                return -1;
            }
            close(pipe_fds[1]);
            
            return 0;
        }
    }
    close(pipe_fds[1]);
    //Close write end as it is unecessary in parent.
    int counts[MAX_WORD_LEN];
    //Initialize array, and make sure that it is initialized to all 0's
    memset(counts, 0,sizeof(int)*MAX_WORD_LEN);
    for(int i=1;i<argc;i++){
        //Not strictly necessary, but considered best practice to wait, because calling read to an empty pipe will wait for it.
        wait(NULL);
    }
    int buffer[MAX_WORD_LEN];
    memset(buffer, 0,sizeof(int)*MAX_WORD_LEN);
    for(int i=1;i<argc;i++){
        while(read(pipe_fds[0], buffer, sizeof(int)*MAX_WORD_LEN)>0){
            //Was told this was adequate error checking, otherwise I would throw in an if ERRVAL != Success, perror("read") return -1 after the for loop. 
            for(int x =0;x<MAX_WORD_LEN;x++){
                counts[x]+=buffer[x];
            } 

        }
    }
    close(pipe_fds[0]);
    //No longer need read, close immediately
    
    for (int j = 1; j <= MAX_WORD_LEN; j++) {
        //print stored values
        printf("%d-Character Words: %d\n", j, counts[j-1]);
    }
    return 0;

    //I was told by TA I did not ever need to error check close() in this project (and also this class as a whole)
}
