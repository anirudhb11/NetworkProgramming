#include "header.h"

int driverProcessPid;
jmp_buf buffA;
/*

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

*/
commandGroup* makeCommand()
{
    commandGroup* cmd =(commandGroup*) malloc(sizeof(commandGroup));
    command_initialization(cmd);

    cmd->command[0] = "ls";
    cmd->argv[0][0] = "ls";


//    cmd->next = NULL;
//    cmd->command[0] = "/bin/ls"; cmd->command[1] = NULL; cmd->command[2] = NULL;
//    cmd->argv[0][0] = "ls";
//    cmd->next = NULL;
    return cmd;

}
/**
 * @return 0 if reading was successful if buffer overflow then -1
*/
int readCmd(char *input){
    int pos = 0;
    char c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
        input[pos] = c;
        pos++;
        if (pos >= BUFFSIZE - 1)
        {
            printf("Input character limit exceeded. Please try again\n");
            return -1;
        }
    }
    input[pos] = '\0';
    return 0;
}

void sigIntHandler(int sig){
    int pgid = getpgid(getpid());
//    printf("PGID: %d DRIVER PID: %d\n", pgid, driverProcessPid);
    if(pgid !=  driverProcessPid){
        tcsetpgrp(STDIN_FILENO, driverProcessPid);
        kill(SIGTERM, -1 * pgid);
    }
    else{
        longjmp(buffA, 0);
    }
}

int main(int argc, char *argv[])
{
    printf("I am here:\n");
    driverProcessPid = getpid();

    char syncChar;

    int a = 1;

    char *input = malloc(sizeof(char) * BUFFSIZE);
    signal(SIGINT, sigIntHandler);
    a = setjmp(buffA);
    printf("Value of a is : %d\n", a);
    while(1){
        int syncPipe[2]; //synchronisation between driver shell process and shell process
        pipe(syncPipe);

        waitpid(-1, NULL, WNOHANG);
        printf("$shell:>");
        if(readCmd(input) < 0)
            continue;

        //parsing functionality to be added here
        commandGroup *cmd = makeCommand();
        pipeline p;
        p.firstCommand = cmd;
        p.numberOfCommands = 1;
        //This is a dummy command

        int childPid = fork();
        if(childPid < 0){
            perror("Error while forking:");
            exit(0);
        }
        else if(childPid == 0){

            close(syncPipe[1]);

            read(syncPipe[0], &syncChar, 1);
            execCommandPipeline(p);
            if(!cmd->isBackground){
                printf("Retuning control:\n");
                tcsetpgrp(STDIN_FILENO, driverProcessPid);
            }
            break;




        }
        else{
            close(syncPipe[0]);
            setpgid(childPid, childPid);
            if(!cmd->isBackground)
                tcsetpgrp(STDIN_FILENO, childPid);
            write(syncPipe[1], "a", 1);

            wait(NULL);
//            printf("Successfully exec");




        }

    }

    

}
