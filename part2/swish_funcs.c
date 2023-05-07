#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"
#include "swish_funcs.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * make use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipes: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipes, int in_idx, int out_idx) {
    if(in_idx!=2*n_pipes){
        //2*n occurs at the first command (input, last child chronologically), which we want to be standard input
        if(dup2(pipes[in_idx], STDIN_FILENO)==-1){
            perror("dup2");
            close(pipes[in_idx]);
            if(out_idx!=-1){
                close(pipes[out_idx]);
            }
            return -1;
        }
    }
    if(out_idx!=-1){
        //-1 is occuring at the last command(input, first child chronologically), which we want to be standard output
        if(dup2(pipes[out_idx], STDOUT_FILENO)==-1){
            perror("dup2");
            close(pipes[out_idx]);
            if(in_idx!=2*n_pipes){
                close(pipes[in_idx]);
            }
            return -1;
        }
    }
    if(run_command(tokens)==-1){
        //if run command returns, that means an error has returned, 
        return -1;
    }
    //this line is superflous
    else{ return 0;}
}

int run_pipelined_commands(strvec_t *tokens) {
    int totalpipes=strvec_num_occurrences(tokens, "|");
    //no need to error check total pipes, #1, because swish will not run if there are no |
    // but also because none of the loops will execute, and the program will simply return if total pipes = 0, or -1
    int *pipe_fds = malloc(2*(totalpipes) * sizeof(int));
    //allocate space for array
    for (int i = 0; i < totalpipes; i++) {
        //create an array of pipes, 
        if (pipe(pipe_fds + 2*i) == -1) {
            perror("pipe");
            for (int j = 0; j < 2*i; j++) {
                close(pipe_fds[j]);
            }
            return -1;
        }
    }
    int startloc =strvec_find_last(tokens, "|")+1;
    //once again, if find last would return an error or -1 for not found, none of the loops would run and it would not matter
    //and also you cannot run pipeline commands in swish.c without at least one | character in tokens.
    //start and end of splice
    int endloc = tokens->length;
    for(int i=0; i<=totalpipes;i++){
        pid_t pid = fork();
        if(pid==0){
            //in child
            for(int j=0;j<2*totalpipes;j++){
                if(j!=2*i){
                    if(j!=2*i-1){
                        //close all unnecesary pipes, only ones that are passed on are 2*i and 2*i-1
                        close(pipe_fds[j]);
                    }
                }
            }
            strvec_t originalcopy;
            //I wanted to do all strvec manipulation outside of the helper function.
            if(strvec_slice(tokens, &originalcopy,startloc,endloc)!=0){
                perror("strvec_slice");
                strvec_clear(&originalcopy);
                close(pipe_fds[2*i]);
                close(pipe_fds[2*i+1]);
                return -1;
            }
            //splice to only have the materials that we want. (From start to |, between | or | to end)
            if(run_piped_command(&originalcopy, pipe_fds, totalpipes, 2*i,2*i-1)==-1){
                //if error occurs, close all and clear all, return -1
                perror("run_command");
                strvec_clear(&originalcopy);
                close(pipe_fds[2*i]);
                close(pipe_fds[2*i+1]);
                return -1;
            }
            else{return 0;}//Superfluous.
            
        }
        else{
            //only thing to do in parent is change splice locations, which means removing the last executed command, 
            //and changing the endloc and startloc
            //strvec take is necessary so that we don't keep finding the same | character.
            strvec_take(tokens, startloc-1);
            endloc = startloc;
            startloc = strvec_find_last(tokens, "|")+1;
        }
    }
    for(int i=0; i<totalpipes;i++){
        //parent has no need of pipes, close immediately
       close(pipe_fds[2*i]);
       close(pipe_fds[2*i+1]);
    }
    for(int i=0; i<=totalpipes;i++){
        wait(NULL);
    }
    //I was told I did not need to error control if one child errored out any more than I did here (IE Wait(&status)) and determine what went wrong, I was told by TA that my above error checking was sufficient.
    
    free(pipe_fds);
    return 0;
}
