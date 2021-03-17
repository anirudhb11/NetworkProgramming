#include "header.h"
//Input redirection only in the start
//output redirection only in the end
//In between pipe you will read and write to pipe

/**
 * @return true if any of the commands part of the command group requires input redirection
*/
bool requiresInputRedirection(commandGroup *cmd){
    return (cmd->inputRedirect[0] || cmd->inputRedirect[1] || cmd->inputRedirect[2]);
}
/**
 * @return true if any of the commands part of the command group requires output redirection
*/
bool requiresOutputRedirection(commandGroup *cmd){
    return (cmd->outputRedirect[0] || cmd->outputAppend[1] || cmd->outputRedirect[1] || cmd->outputAppend[1] ||
            cmd->outputRedirect[2] || cmd->outputAppend[2]);
}

/**
 * Executes one command of the pipeline
 */
void executeCommandGroup(commandGroup cmd, int *readPipes, int *writePipes){
    int ret = fork();
    if(ret == -1){
        perror("Forking failed:");
    }
    else if(ret == 0){
        char *buff = (char *)malloc(sizeof(char) * 512);
        int readBytes;
        if(readPipes != NULL){
            readBytes = read(readPipes[0], buff, 512);
            if(readBytes < 0){
                perror("Error reading data\n");
            }
            buff[readBytes] = '\0';
//            printf("READ %s\n", buff);
        }

        int tempPipe[2];
        for(int subCommandIndex = 0; subCommandIndex < cmd.pipeType; subCommandIndex++){
            pipe(tempPipe);
            int ret = fork();
            if(ret < 0){
                perror("Forking failed while executing subcommand:");
            }
            if(ret == 0){
                if(close(tempPipe[1]) < 0){
                    printf("Couldn't close temporary pipe\n");
                }
                if(cmd.inputRedirect[subCommandIndex] == true){
                    int fd = open(cmd.inputFilename[subCommandIndex], O_RDONLY);
                    if(fd < 0){
                        perror("Failed to open file to read:");
                    }
                    dup2(fd, STDIN_FILENO);
                }
                else{
                    dup2(tempPipe[0], STDIN_FILENO);
                }

                if(subCommandIndex == cmd.pipeType - 1){ //Last command of the group, it's output must be written to writePipes
                    if(cmd.outputRedirect[subCommandIndex] == true || cmd.outputAppend[subCommandIndex] == true){
                        int fd;
                        if(cmd.outputRedirect[subCommandIndex] == true) // Redirect mode >
                            fd = open(cmd.outputFilename[subCommandIndex], O_CREAT | O_WRONLY);
                        else // Append mode >>
                            fd = open(cmd.outputFilename[subCommandIndex], O_CREAT | O_WRONLY | O_APPEND);

                        if(fd < 0){
                            perror("Failed to open file to write:");
                        }
                        dup2(fd, STDOUT_FILENO);

                    }
                    else if(writePipes != NULL){
                        dup2(writePipes[1], STDOUT_FILENO);
                    }
                }

                execv(cmd.command[subCommandIndex], cmd.argv[subCommandIndex]);

            }
            else{
                //Read data from prev pipe and then write it into this pipe

                close(tempPipe[0]);
                write(tempPipe[1], buff, readBytes);
                close(tempPipe[1]);
                wait(NULL);

            }
        }
        exit(0);

    }
    else{
        wait(NULL);
    }

}


/**
 * Executes the entire command pipeline
 */
void execCommandPipeline(pipeline cmdPipeline){
    int numCommands = cmdPipeline.numberOfCommands;
    int pipes[numCommands - 1][2];  //for n commands we need n - 1 pipes

    for(int pipeIndex=0;pipeIndex<numCommands -1; pipeIndex++){
        pipe(pipes[pipeIndex]);
    }

    commandGroup *cmd = cmdPipeline.firstCommand;
    int cmdIndex = 0;
    while(cmd != NULL){
        if(cmdIndex != 0 && requiresInputRedirection(cmd)){
            printf("INVALID COMMAND: Input redirection only allowed at the beginning of pipeline\n");
            return;
        }
        if(cmdIndex != numCommands - 1 && requiresOutputRedirection(cmd)){
            printf("INVALID COMMAND: Output redirection only allowed at the end of pipeline\n");
            return;
        }

        int *readPipeFd = NULL; //FDs which command will use to read from
        int *writePipeFd = NULL; //FDs which command will use to write to
        if(cmdIndex > 0){
            readPipeFd = pipes[cmdIndex - 1];
        }
        if(cmdIndex < numCommands - 1){
            writePipeFd = pipes[cmdIndex];
        }
        executeCommandGroup(*cmd, readPipeFd, writePipeFd);
//        printf("Executed command %s\n", cmd->argv[0][0]);
        cmd = cmd->next;
        cmdIndex++;
    }
}

void command_initialization( commandGroup *cmd){
    for(int cmd_index = 0; cmd_index < 3; cmd_index++){
        cmd->command[cmd_index] = NULL;
        for(int arg_index = 0; arg_index < 10; arg_index++){
            cmd->argv[cmd_index][arg_index] = NULL;
        }
        cmd->pipeType = 1;
        cmd->inputRedirect[cmd_index] = false;
        cmd->outputAppend[cmd_index] = false;
        cmd->outputAppend[cmd_index] = false;
        cmd->isBackground[cmd_index] = false;
        cmd->inputFilename[cmd_index] = NULL;
        cmd->outputFilename[cmd_index] = NULL;
        cmd->next = NULL;


    }
}

int main(){
    //ls ||| wc, wc, pwd | wc execution
    pipeline p;
    p.numberOfCommands = 3;

    commandGroup c1, c2, c3;
    command_initialization(&c1);
    command_initialization(&c2);
    command_initialization(&c3);

    c1.command[0] = (char *)malloc(sizeof(char) *20);
    c1.argv[0][0] = (char *)malloc(sizeof(char) * 20);
    c1.argv[0][1] = (char *)malloc(sizeof(char) * 20);

    c2.command[0] = (char *)malloc(sizeof(char) * 20);
    c2.argv[0][0] = (char *)malloc(sizeof(char) * 20);

    c2.command[1] = (char *)malloc(sizeof(char) * 20);
    c2.argv[1][0] = (char *)malloc(sizeof(char) * 20);

    c2.command[2] = (char *)malloc(sizeof(char) * 20);
    c2.argv[2][0] = (char *)malloc(sizeof(char) * 20);

    c3.command[0] = (char *)malloc(sizeof(char) *20);
    c3.argv[0][0] = (char *)malloc(sizeof(char) *20);


    strcpy(c1.command[0], "/bin/ls");
    strcpy(c1.argv[0][0], "ls");
    strcpy(c1.argv[0][1], "-l");

    strcpy(c2.command[0], "/usr/bin/wc");
    strcpy(c2.argv[0][0], "wc");

    strcpy(c2.command[1], "/usr/bin/wc");
    strcpy(c2.argv[1][0], "wc");

    strcpy(c2.command[2], "/bin/pwd");
    strcpy(c2.argv[2][0], "pwd");

    strcpy(c3.command[0], "/bin/pwd");
    strcpy(c3.argv[0][0], "pwd");

    p.firstCommand = &c1;
    c1.next = &c2;
    c2.next = &c3;
    c2.pipeType=3;

//    execCommandPipeline(p);

    //Example 2
    //ls -l || grep ^d, grep ^-
    pipeline p2;
    p2.numberOfCommands = 2;

    commandGroup c4, c5;
    command_initialization(&c4);
    command_initialization(&c5);
    c5.pipeType = 2;

    c4.command[0] = (char *)malloc(sizeof(char) *20);
    c4.argv[0][0] = (char *)malloc(sizeof(char) * 20);
    c4.argv[0][1] = (char *)malloc(sizeof(char) * 20);

    c5.command[0] = (char *)malloc(sizeof(char) * 20);
    c5.argv[0][0] = (char *)malloc(sizeof(char) * 20);
    c5.argv[0][1] = (char *)malloc(sizeof(char) * 20);

    c5.command[1] = (char *)malloc(sizeof(char) * 20);
    c5.argv[1][0] = (char *)malloc(sizeof(char) * 20);
    c5.argv[1][1] = (char *)malloc(sizeof(char) * 20);

    strcpy(c4.command[0], "/bin/ls");
    strcpy(c4.argv[0][0], "/ls");
    strcpy(c4.argv[0][1], "-l");

    strcpy(c5.command[0], "/bin/grep");
    strcpy(c5.argv[0][0], "grep");
    strcpy(c5.argv[0][1], "^-");

    strcpy(c5.command[1], "/bin/grep");
    strcpy(c5.argv[1][0], "grep");
    strcpy(c5.argv[1][1], "root");

    p2.firstCommand = &c4;
    c4.next = &c5;
//    execCommandPipeline(p2);

    //Example 3
    //wc < shell.c
    pipeline p3;
    p3.numberOfCommands = 1;

    commandGroup c6;
    command_initialization(&c6);

    c6.inputRedirect[0] = true;
    c6.inputFilename[0] = (char *)malloc(sizeof(char) *20);

    c6.command[0] = (char *)malloc(sizeof(char) * 20);
    c6.argv[0][0] = (char *)malloc(sizeof(char) * 20);

    strcpy(c6.command[0], "/usr/bin/wc");
    strcpy(c6.argv[0][0], "wc");
    strcpy(c6.inputFilename[0], "shell.c");

    p3.firstCommand = &c6;
//    execCommandPipeline(p3);

    //example 4
    //wc < shell.c
    pipeline p4;
    p4.numberOfCommands = 1;

    commandGroup c7;
    command_initialization(&c7);

    c7.inputRedirect[0] = true;
    c7.outputAppend[0] = true;
    c7.inputFilename[0] = (char *)malloc(sizeof(char) *20);
    c7.outputFilename[0] = (char *)malloc(sizeof(char) *20);

    c7.command[0] = (char *)malloc(sizeof(char) * 20);
    c7.argv[0][0] = (char *)malloc(sizeof(char) * 20);

    strcpy(c7.command[0], "/usr/bin/wc");
    strcpy(c7.argv[0][0], "wc");
    strcpy(c7.inputFilename[0], "shell.c");
    strcpy(c7.outputFilename[0], "abc.txt");

    p4.firstCommand = &c7;
    execCommandPipeline(p4);









}