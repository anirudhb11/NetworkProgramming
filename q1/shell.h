#define MAX_ARGS 10 //maximum number of arguments that can be passed to a command
#define MAX_ARG_LEN 20 //maximum length of a single argument
#define MAX_TO_PIPE_COMMANDS 3 //maximum number of commands to which the output of a command needs to be piped
#define MAX_FILENAME_LEN 50 //maximum length of the file name

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

struct command{
    char *com_name[MAX_TO_PIPE_COMMANDS]; //name of the command to run
    char *argv[MAX_TO_PIPE_COMMANDS][MAX_ARGS] ;//arguments passed to this command terminated NULL, 1st argument is com_name
    int num_args[MAX_TO_PIPE_COMMANDS]; //number of arguments for this command
    bool input_redirected[MAX_TO_PIPE_COMMANDS]; //true if command requires input redirection
    bool output_redirected[MAX_TO_PIPE_COMMANDS]; //true if command requires output redirection
    bool output_append[MAX_TO_PIPE_COMMANDS]; //true if command requires output to be appended
    char *input_file[MAX_TO_PIPE_COMMANDS]; //if command requires input redirection stores the name of the file
    char *output_file[MAX_TO_PIPE_COMMANDS]; //if command requires output redirection/ output appending stores the name of the output file
    bool is_background[MAX_TO_PIPE_COMMANDS]; //true is command needs to be run in the background

};

typedef struct command command;