#include "header.h"
commandGroup* getNewCommand()
{
    commandGroup *c = (commandGroup *)malloc(sizeof(commandGroup));
    return c;
}
void handleLine(char** tokens, commandGroup* cmd, int pipeNum, int cmdNum)
{
    
    
    if (search(tokens[cmdNum], '<'))
    {
        //printf("HI \n");

        int pos = charPos(tokens[cmdNum], '<');
        int l = strlen(tokens[cmdNum]);
        if (pos < l - 1)
            cmd->inputFilename[0] = slicestring(pos + 1, l - 1, tokens[cmdNum]);
        cmd->command[pipeNum] = slicestring(0, pos - 1, tokens[cmdNum]);
        cmd->argv[pipeNum] = slicestring(0, pos - 1, tokens[cmdNum]);
    }
    else if (search(tokens[cmdNum], '>'))
    {
        int pos = charPos(tokens[cmdNum], '>');
        int l = strlen(tokens[cmdNum]);
        if (pos < l - 1 && tokens[cmdNum][pos + 1] != '>')
            cmd->outputFilename[0] = slicestring(pos + 1, l - 1, tokens[cmdNum]);
        else if (pos + 1 < l - 1 && tokens[cmdNum][pos + 1] == '>')
            cmd->outputAppend[0] = slicestring(pos + 2, l - 1, tokens[cmdNum]);

        cmd->command[pipeNum] = slicestring(0, pos - 1, tokens[cmdNum]);
        cmd->argv[pipeNum] = slicestring(0, pos - 1, tokens[cmdNum]);
    }
    else
    {
        cmd->command[pipeNum] = tokens[cmdNum];
        cmd->argv[pipeNum] = tokens[cmdNum];
    }
    int i = 1, j = 1;

    //printf("New Command is %s \n" ,cmd->command[pipeNum]);
    
    while (tokens[i] != NULL)
    {
        if (tokens[i][0] == '-')
            cmd->argv[j++] = slicestring(1, strlen(tokens[i]) - 1, tokens[i]);
        else if (tokens[i][0] == '<')
        {
            if (strlen(tokens[i]) == 1)
            {
                cmd->inputFilename[pipeNum] = tokens[i + 1];
                cmd->inputRedirect[pipeNum] = true;
                i++;
            }
            else
                cmd->inputFilename[0] = slicestring(1, strlen(tokens[i]) - 1, tokens[i]);
        }
        else if (tokens[i][0] == '>')
        {
            if (strlen(tokens[i]) == 1)
            {
                cmd->outputFilename[pipeNum] = tokens[i + 1];
                cmd->outputRedirect[pipeNum] = true;
                i++;
            }
            else if (tokens[i][1] != '>')
            {
                cmd->outputFilename[pipeNum] = slicestring(1, strlen(tokens[i]) - 1, tokens[i]);
                cmd->outputRedirect[pipeNum] = true;
            }
            else
            {
                if (strlen(tokens[i]) == 2)
                {
                    cmd->outputFilename[pipeNum] = tokens[i + 1];
                    cmd->outputRedirect[pipeNum] = true;
                    i++;
                }
                else
                {
                    cmd->outputAppend[pipeNum] = true;
                    cmd->outputFilename[pipeNum] = slicestring(2, strlen(tokens[i]) - 1, tokens[i]);
                }
            }
        }
        i++;
    }

    if( search(tokens[pipeNum] , '&'))
        cmd->isBackground[pipeNum] = true;
    
}
commandGroup* noPipe(char *inp, int left, int right)
{
    commandGroup *cmd = getNewCommand();
    cmd->pipeType = 0;

    /*
    bool bg = search(inp, '&');
    for (int i = 0; i < 3; i++)
        cmd->isBackground[i] = bg;
    */
    char **tokens = tokenize(slicestring(left, right, inp));
    handleLine(tokens, cmd, 0, 0);
    //printf("Last Command is %s \n", cmd->command[0]);

    return cmd;
}
commandGroup* singlePipe(char *inp, int left, int right)
{
    commandGroup* cmd = getNewCommand();
    cmd->pipeType = 1;
    
    /*
    bool bg = search(inp, '&');
    for (int i = 0; i < 3; i++)
        cmd->isBackground[i] = bg;
    */
    char** tokens = tokenize( slicestring(left , right , inp )) ;
    handleLine(tokens, cmd, 0 , 0);
    
    
    return cmd;
}
commandGroup* doublePipe(char *inp, int left, int right)
{
    commandGroup *cmd = getNewCommand();
    cmd->pipeType = 2;



    return cmd;
    
}
commandGroup* triplePipe(char *inp, int left, int right)
{
    commandGroup *cmd = getNewCommand();
    cmd->pipeType = 1;

    return cmd;
    
}

commandGroup* parseInput(char* inp)
{
    
    commandGroup *head = getNewCommand();
    commandGroup *tmp = head;
    int i = 0;
    int left = 0;
    
    while (inp[i] != '\0')
    {
        if (inp[i] == '|')
        {
            if ((i + 1) < strlen(inp) && inp[i + 1] == '|')
            {
                if ((i + 2) < strlen(inp) && inp[i + 2] == '|')
                {
                    //triple pipe
                    //printf("Triple Pipe Detected \n");
                    tmp->next = triplePipe(inp, left, i - 1);
                    left = i + 3;
                    i += 2;
                    
                }
                else
                {
                    //double pipe
                    //printf("Double Pipe Detected \n");
                    tmp->next = doublePipe(inp, left, i - 1);
                    left = i + 2;
                    i += 1;
                }
            }
            else
            {
                //printf("Single Pipe Detected \n");
                tmp->next = singlePipe(inp, left, i - 1);
                left = i + 1;
            }

            
            tmp = tmp->next;
        }
        i++;
    }
    
    //printf("%d, %d \n", left , i );

    tmp->next = noPipe(inp , left , i - 1);


    return head->next;

}



