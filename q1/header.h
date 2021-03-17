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
    int pipeType; //number of commands in a command group, (can take 1/2/3)
    bool inputRedirect[3];
    bool outputRedirect[3];
    bool outputAppend[3];
    char *inputFilename[3];
    char *outputFilename[3];
    bool isBackground[3];

    struct commandGroup* next;
} commandGroup;

typedef struct pipeline{
    commandGroup *firstCommand;
    int numberOfCommands;
}pipeline;

commandGroup* parseInput(char** arg);

int singlePipe(char **ar1, char **ar2);
int doublePipe(char **ar1, char **ar2);
int triplePipe(char **ar1, char** ar2);
int execCommand(commandGroup* cmd);










