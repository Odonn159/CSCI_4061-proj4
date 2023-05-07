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
    if(in_idx!=0){
        dup2(pipes[in_idx], STDOUT_FILENO);
    }
    if(out_idx!=2*n_pipes+1){
        dup2(pipes[out_idx], STDIN_FILENO);
    }
    printf("L34FUCK");
    return run_command(tokens);
}

int run_pipelined_commands(strvec_t *tokens) {
    int totalpipes=strvec_num_occurrences(tokens, "|");
    int *pipe_fds = malloc(2*(totalpipes) * sizeof(int));
    for (int i = 0; i < totalpipes; i++) {
        if (pipe(pipe_fds + 2*i) == -1) {
            perror("pipe");
            for (int j = 0; j < 2*i; j++) {
                close(pipe_fds[j]);
            }
            return -1;
        }
    }
    int startloc =strvec_find_last(tokens, "|")+1;
    int endloc = tokens->length;
    for(int i=totalpipes; i>=0;i--){
        pid_t pid = fork();
        if(pid==0){
            for(int j=0;j<totalpipes;j++){
                if(j!=i){
                    close(pipe_fds[2*j]);
                    close(pipe_fds[2*j+1]);
                }
            }
            strvec_t originalcopy;
            strvec_slice(tokens, &originalcopy,startloc,endloc);
            if(run_piped_command(&originalcopy, pipe_fds, totalpipes, 2*i,2*i+1)){
                perror("run_command");
                strvec_clear(&originalcopy);
                close(pipe_fds[2*i]);
                close(pipe_fds[2*i+1]);
                return -1;
            }
            
        }
        else{
            strvec_take(tokens, startloc-1);
            endloc = startloc;
            startloc = strvec_find_last(tokens, "|")+1;
        }
    }
    for(int i=0; i<totalpipes;i++){
       close(pipe_fds[2*i]);
       close(pipe_fds[2*i+1]);
    }
    for(int i=totalpipes; i>0;i--){
          wait(NULL);
    }
    free(pipe_fds);
    return 0;
}
