#include "header.h"

commandGroup* scTable[MAX_SHORTCUTS];

char *cwd;


pid_t driver_process_pid;
jmp_buf buffA;

int sig_int_handler_pipe[2];

void printCommandGrp(commandGroup* cmd)
{
    //printf("IS BG: %d\n", cmd->isBackground);
    
    printf("COMMANDS: ");
    int i = 0;
    while(cmd->command[i] ) 
        printf("%d. %s," ,i  , (cmd->command[i++] ) );
    printf("\n");

    printf("ARGS : ");
    i = 0;
    
    for (int j = 0; j < 3; j++)
    {
        int k = 0;
        while (cmd->argv[j][k])
            printf("%d.%d %s,", j + 1, k, cmd->argv[j][k++]);
    }


    printf("\n");

    /*
    
    */
    printf("BACKGROUND PROCESS: %d \n" , cmd->isBackground);
   

    printf("INP REDIR: ");
    i = 0;
    while (cmd->inputRedirect[i])
        printf("%d. %d,",i, cmd->inputRedirect[i++]);
    printf("\n");

    printf("INP FNAME: ");
    i = 0;
    while (cmd->inputFilename[i])
        printf("%d. %s,", i , cmd->inputFilename[i++]);
    printf("\n");


    printf("OUTPUT REDIR: ");
    i = 0;
    while (cmd->outputRedirect[i])
        printf("%d. %d,",i , cmd->outputRedirect[i++]);
    printf("\n");

    printf("OUTPUT APPEND: ");
    i = 0;
    while (cmd->outputAppend[i])
        printf("%d. %d,", i, cmd->outputAppend[i++]);
    printf("\n");

    printf("OP FNAME: ");
    i = 0;
    while (cmd->outputFilename[i])
        printf("%d. %s ,", i , cmd->outputFilename[i++]);
    printf("\n ********************************** \n");

}




void sigHandler(int sig){
    if(getpid() == tcgetpgrp(STDIN_FILENO)){
        tcsetpgrp(STDIN_FILENO, driver_process_pid);
        kill(SIGUSR1, driver_process_pid);
        exit(0);
    }
    else{
        exit(0);
    }

}


int find_num_commands(commandGroup *first_command){
    int num_commands =0;
    while(first_command != NULL){
        num_commands++;
        first_command = first_command->next;
    }
    return num_commands;
}

commandGroup *getLastCommandGroup(commandGroup *head){
    while(head->next != NULL){
        head = head->next;
    }
    return head;
}


int callExecutor(pipeline exec_pipeline, char *cwd, commandGroup *lastCmd){
    int sync_pipe[2]; //synchronisation between driver shell process and shell process
    pipe(sync_pipe);

    char sync_char;
    int child_pid = fork();
    if(child_pid < 0){
        perror("Error while forking:");
        exit(0);
    }
    else if(child_pid == 0){
        signal(SIGINT, sigHandler);
        signal(SIGQUIT, sigHandler);

        close(sync_pipe[1]);
        read(sync_pipe[0], &sync_char, 1);
//            printf("Executor PID : %d PGID %d\n", getpid(), getpgid(getpid()));
        execCommandPipeline(exec_pipeline, cwd);
        if(!lastCmd->isBackground){
//            printf("Returning control:\n");
            tcsetpgrp(STDIN_FILENO, driver_process_pid);
        }
        return -1;

    }
    else{

        close(sync_pipe[0]);
        setpgid(child_pid, child_pid);
        if(!lastCmd->isBackground){
            tcsetpgrp(STDIN_FILENO, child_pid);
        }

        write(sync_pipe[1], "a", 1);
        close(sync_pipe[1]);
        int status;

        if(lastCmd->isBackground){
            int child_waiter = fork();
            if(child_waiter == 0) {
                waitpid(child_pid, &status, 0);
                return -1;
            }
        }
        else{
            waitpid(child_pid, &status, 0);
            printf("Process terminated with status %d\n", status);
        }
    }


}
void shortCutHandler(int sig)
{
    printf("\nEntered short cut mode\n");
    int scNumber;
    scanf("%d", &scNumber);
    if (scTable[scNumber] == NULL)
    {
        printf("No entry exists for index. %d \n", scNumber);
        longjmp(buffA, 1);
    }
    if (scNumber < 0 || scNumber > 999)
    {

        printf("%d: Input Number limit exceeded.\n", scNumber);
        longjmp(buffA, 1);
    }


    printf("SC:%d Executing \n", scNumber);
    pipeline exec_pipeline;
    exec_pipeline.firstCommand = scTable[scNumber];
    exec_pipeline.numberOfCommands = find_num_commands(scTable[scNumber]);
    commandGroup *lastCommand = getLastCommandGroup(scTable[scNumber]);
    callExecutor(exec_pipeline, cwd, lastCommand);
    longjmp(buffA, 1);
}



int main(int argc, char *argv[])
{
    signal(SIGINT, shortCutHandler);
    for(int pos =0; pos < MAX_SHORTCUTS; pos++){
        scTable[pos] = NULL;
    }

    int shm_id = shmget(SHM_KEY, 1024, 0666|IPC_CREAT);
    cwd = (char*) shmat(shm_id,(void*)0,0);
    getcwd(cwd, 1024);


    printf("Current working directory: %s\n", cwd);
    driver_process_pid = getpid();
    printf("Driver PID: %d\n", driver_process_pid);

    int a = 1;

    a = setjmp(buffA);
    while (1)
    {

        waitpid(-1, NULL, WNOHANG);

        char cur_dir[1024];
        getcwd(cur_dir, sizeof(cur_dir));
        printf("\n$shell:>");
        fflush(stdout);

        char *input = malloc(sizeof(char) * BUFFSIZE);
        int pos = 0;
        char c;
        while ((c = getchar()) != '\n' && c != EOF)
        {
            input[pos] = c;
            pos++;
            if (pos >= BUFFSIZE - 1)
            {
                printf("Input character limit exceeded. Please try again\n");
                break;
            }
        }

        if( pos == 0 || pos >= BUFFSIZE - 1) continue;
        input[pos] = '\0';

        input = trimwhitespace(input);
        char tmp[BUFFSIZE]; strcpy(tmp , input);
        int inpLen = strlen(input);
        if( input[0] == 's' && input[1] == 'c'){
            //printf("Shortcut Command \n");
            char **tokens = tokenize(input, WHITESPACE_DELIM);
            int j = 1;
            if( tokens[j][0] == '-' && tokens[j][1] == 'i')
            {
                printf("%s \n", tokens[j + 1]);
                int num = atoi(tokens[j + 1]);
                int len = strlen(tokens[j + 1]) + 7;
                char* str = slicestring(len, inpLen , tmp);
                printf("%s is stored in %d \n", str, num);
                scTable[num] = parseInput(str);
            }
            else if (tokens[j][0] == '-' && tokens[j][1] == 'd')
            {
                int num = atoi(tokens[j + 1]);
                printf("%s stored in %d is Deleted\n", scTable[num]->command[0], num);
                scTable[num] = NULL;
            }
            continue;
        }
        if( !input) continue;

        commandGroup* cmd = parseInput(input);

        pipeline exec_pipeline;
        exec_pipeline.firstCommand = cmd;
        exec_pipeline.numberOfCommands = find_num_commands(cmd);
        commandGroup *lastCommand = getLastCommandGroup(cmd);
        chdir(cwd);

        int ret = callExecutor(exec_pipeline, cwd, lastCommand);
        if(ret == -1){
            break;
        }
    }

    
    return 0;
}
