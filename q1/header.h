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
#define MAX_ARGS 3
#define MAX_ARGLEN 128
#define WHITESPACE_DELIM " \t\r\n\a"

typedef struct commandGroup {
    char *command[3];
    char *argv[MAX_ARGS];
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

commandGroup* parseInput(char* arg);
bool search(char *inp, char ch);
int charPos(char *inp, char ch);
char *slicestring(int left, int right, char *inp);
char *trimwhitespace(char *str);
char **tokenize(char *line);

int execCommand(commandGroup *cmd);
