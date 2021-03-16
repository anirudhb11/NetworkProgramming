#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFSIZE 1024
#define CMDSIZE 128
#define MAX_ARGS 2
#define MAX_ARGLEN 128

typedef struct commandGroup {
    char *command[3];
    char *argv[3][MAX_ARGS];
    //int pipeType; // 0: no pipe 1: single pipe  2: double pipe 3: triple pipe
    bool inputRedirect[3];
    bool outputRedirect[3];
    bool outputAppend[3];
    int inputFilename[3][MAX_ARGLEN];
    int outputFilename[3][MAX_ARGLEN];
    bool isBackground;

    struct commandGroup* next;
} commandGroup;

commandGroup* parseInput(char** arg);

int singlePipe(char **ar1, char **ar2);
int doublePipe(char **ar1, char **ar2);
int triplePipe(char **ar1, char** ar2);
int execCommand(commandGroup* cmd);










