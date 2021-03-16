#include "header.h"

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


    cmd->next = NULL;
    cmd->command[0] = "/bin/ls"; cmd->command[1] = NULL; cmd->command[2] = NULL;
    cmd->argv[0][0] = "ls";
    cmd->next = NULL;

  
    return cmd;

}
int main(int argc, char *argv[])
{
    pid_t pid;
    /*
    signal(SIGINT, sig_handler);
    signal(SIGTTOU, SIG_IGN);
    */

    pid = tcgetpgrp(STDOUT_FILENO);

    setpgid(getpid(), 0);
    tcsetpgrp(STDOUT_FILENO, getpid());

    int a = 1;
    while (a)
    {
        char cur_dir[1024];
        getcwd(cur_dir, sizeof(cur_dir));
        printf("$shell:>");

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
        if( pos >= BUFFSIZE - 1) continue;


        input[pos] = '\0';

        int i = 0;
        char **aarg = malloc(sizeof(char) * BUFFSIZE);
        const char *space = " ";
        char *arg = strtok(input, space);

        
        while (arg != NULL)
        {
            aarg[i] = arg;
            i++;
            arg = strtok(NULL, space);
        }
        aarg[i] = NULL;
        /*

        int j = 0;
        while( aarg[j] != NULL)
        {
            printf("%s \n" , aarg[j]);
            j++;
        }
        */
        
        
        /*

        commandGroup* cmd = parseInput(aarg);
        a = execCommand(cmd);
        
        */

        a = execCommand(makeCommand());
        a = 1;

        //printf("%d \n", a);
    }

    
    return 1;
}
