#include "header.h"
void printCommandGrp(commandGroup* cmd)
{
    //printf("IS BG: %d\n", cmd->isBackground);
    
    printf("COMMANDS: ");
    int i = 0;
    while(cmd->command[i] ) 
        printf("%d. %s," ,i  , (cmd->command[i++] ) );
    printf("\n");

/*
    printf("ARGS : ");
    i = 0;
    while (cmd->argv[i])
        printf("%d. %s ,", i + 1, cmd->argv[i++]);
    printf("\n");

*/

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

        if( pos == 0 || pos >= BUFFSIZE - 1) continue;

        input = trimwhitespace(input);
        if( input[0] == 's' && input[1] == 'c'){
            printf("Shortcut Command \n");
            continue;
        }

        



        input[pos] = '\0';

        /*
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
        
        
        
        

        commandGroup* cmd = parseInput(aarg);
        a = execCommand(cmd);
        
        */

        commandGroup* cmd = parseInput(input);
        
        
        while(cmd)
        {
            
            printCommandGrp(cmd);
            cmd = cmd->next;
        }
        /**/
    }

    
    return 1;
}
