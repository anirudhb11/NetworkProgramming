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
#include <setjmp.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <assert.h>
#define BUFFSIZE 1024
#define CMDSIZE 128
#define MAX_ARGS 3
#define MAX_ARGLEN 128
#define WHITESPACE_DELIM " \t\r\n\a"
#define COMMA_DELIM ","
#define SHM_KEY 636
#define MAX_SHORTCUTS 1000

typedef struct commandGroup {
    char *command[3];
    char *argv[3][MAX_ARGS];
    int pipeType; //number of commands in a command group, (can take 1/2/3)
    bool inputRedirect[3];
    bool outputRedirect[3];
    bool outputAppend[3];
    char *inputFilename[3];
    char *outputFilename[3];
    bool isBackground;
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
char **tokenize(char *line, char* delim);
char *findPath(char *token0) ;
int execCommand(commandGroup *cmd);
void shortCutHandler(int sig);
int callExecutor(pipeline exec_pipeline, char *cwd, commandGroup *lastCmd);
commandGroup *getLastCommandGroup(commandGroup *head);
int find_num_commands(commandGroup *first_command);
void sigHandler(int sig);
void printCommandGrp(commandGroup* cmd);


bool requiresInputRedirection(commandGroup *cmd);
bool requiresOutputRedirection(commandGroup *cmd);
void executeCommandGroup(commandGroup cmd, int *readPipes, int *writePipes, char *cwd);
void execCommandPipeline(pipeline cmdPipeline, char *cwd);
void command_initialization( commandGroup *cmd);
