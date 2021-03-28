#include "header.h"
commandGroup** scTable;

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

int find_num_commands(commandGroup *first_command){
    int num_commands =0;
    while(first_command != NULL){
        num_commands++;
        first_command = first_command->next;
    }
    return num_commands;
}
void shortCutHandler()
{
    printf("\nEntered short cut mode\n");
    int scNumber;
    scanf("%d", &scNumber);
    if (scTable[scNumber] == NULL)
    {
        printf("No entry exists for index. %d \n", scNumber);
        //exit(0);
        return;
    }
    if (scNumber < 0 || scNumber > 999)
    {

        printf("%d: Input Number limit exceeded.\n", scNumber);
        //exit(0);
        return;
    }

    /*
    printf("SC:%d Executing \n", scNumber);
    int isBackground = scTable[scNumber]->isBackground;
    int pid = fork();
    pipeline exec_pipeline;
    exec_pipeline.firstCommand = scTable[scNumber];
    exec_pipeline.numberOfCommands = find_num_commands(scTable[scNumber]);

    execCommandPipeline(exec_pipeline);
    */

}
int main(int argc, char *argv[])
{
    pid_t pid;
    
    signal(SIGINT, shortCutHandler);
    signal(SIGTTOU, SIG_IGN);
    
    
    pid = tcgetpgrp(STDOUT_FILENO);

    setpgid(getpid(), 0);
    tcsetpgrp(STDOUT_FILENO, getpid());
    commandGroup* scLookup[1000]; int lastSC = 0; scTable = scLookup;

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
                scLookup[num] = parseInput(str);
            }
            else if (tokens[j][0] == '-' && tokens[j][1] == 'd')
            {
                int num = atoi(tokens[j + 1]);
                printf("%s stored in %d is Deleted\n", scLookup[num]->command[0], num);
                scLookup[num] = NULL;
            }
            continue;
        }
        if( !input) continue;

        commandGroup* cmd = parseInput(input);
        commandGroup *cmd2 = cmd;
        
        while(cmd2)
        {

            printCommandGrp(cmd2);
            cmd2 = cmd2->next;
        }

        pipeline exec_pipeline;
        exec_pipeline.firstCommand = cmd;
        exec_pipeline.numberOfCommands = find_num_commands(cmd);

        execCommandPipeline(exec_pipeline);
    }

    
    return 1;
}
