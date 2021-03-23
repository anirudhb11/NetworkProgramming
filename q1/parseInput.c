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
        int pos = charPos(tokens[cmdNum], '<');
        int l = strlen(tokens[cmdNum]);
        if (pos < l - 1)
            cmd->inputFilename[0] = slicestring(pos + 1, l - 1, tokens[cmdNum]);
        cmd->command[pipeNum] = findPath(slicestring(0, pos - 1, tokens[cmdNum]));
        cmd->argv[pipeNum][0] = slicestring(0, pos - 1, tokens[cmdNum]);
    }
    else if (search(tokens[cmdNum], '>'))
    {
        int pos = charPos(tokens[cmdNum], '>');
        int l = strlen(tokens[cmdNum]);
        if (pos < l - 1 && tokens[cmdNum][pos + 1] != '>')
            cmd->outputFilename[0] = slicestring(pos + 1, l - 1, tokens[cmdNum]);
        else if (pos + 1 < l - 1 && tokens[cmdNum][pos + 1] == '>')
            cmd->outputAppend[0] = slicestring(pos + 2, l - 1, tokens[cmdNum]);

        cmd->command[pipeNum] = findPath(slicestring(0, pos - 1, tokens[cmdNum]));
        cmd->argv[pipeNum][0] = slicestring(0, pos - 1, tokens[cmdNum]);
    }
    else
    {
        cmd->command[pipeNum] = findPath(tokens[cmdNum]);
        cmd->argv[pipeNum][0] = tokens[cmdNum];
    }
    int i = 1, j = 1;

    //printf("New Command is %s \n" ,cmd->command[pipeNum]);
    
    while (tokens[i] != NULL)
    {
        if (tokens[i][0] == '-')
            cmd->argv[pipeNum][j++] = slicestring(1, strlen(tokens[i]) - 1, tokens[i]);
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
        cmd->isBackground = true;
    
}
commandGroup* noPipe(char *inp, int left, int right)
{
    commandGroup *cmd = getNewCommand();
    cmd->pipeType = 0;

    char **tokens = tokenize(slicestring(left, right, inp), WHITESPACE_DELIM);
    handleLine(tokens, cmd, 0, 0);
    
    return cmd;
}
commandGroup* multiPipe(char *inp, int left, int right, commandGroup* tail)
{
    commandGroup *cmd = tail;
    commandGroup* tmp = getNewCommand();
    //printf("%dP %d, %d \n", tail->pipeType, left , right);
    char **tokens;

    if(!left)
    {
        tokens = tokenize(trimwhitespace(slicestring(left,right, inp)), WHITESPACE_DELIM);
        handleLine(tokens, cmd , 0, 0);
    }

    int i = right + cmd->pipeType + 1;
    while( i < strlen(inp) && inp[i] != '\0' && inp[i] != '|') i++;

    char* sub = trimwhitespace(slicestring(right + cmd->pipeType + 1, i - 1, inp) );

    //printf("%dP %d, %d \n", tail->pipeType, right + cmd->pipeType + 1, i - 1);

    tokens = tokenize(sub , COMMA_DELIM);
    int j = 0;
    while( tokens[j] && j < cmd->pipeType)
    {
        //printf("%s, ", tokens[j]);
        char **ttok = tokenize(trimwhitespace(tokens[j]), WHITESPACE_DELIM);
        handleLine(ttok, tmp, j, j);

        j++;
    }
    printf("\n");

    cmd->next = tmp;

    return cmd;
}
commandGroup* parseInput(char* inp)
{
    commandGroup *head = getNewCommand();
    commandGroup *tmp = head; tmp->pipeType = 0;
    int i = 0;
    int left = 0;
    
    while (i < strlen(inp) && inp[i] != '\0')
    {
        if (inp[i] == '|')
        {
            if ((i + 1) < strlen(inp) && inp[i + 1] == '|')
            {
                if ((i + 2) < strlen(inp) && inp[i + 2] == '|')
                {
                    //triple pipe
                    if( left ) left += 3;
                    
                    tmp->pipeType = 3;
                    multiPipe(inp, left, i - 1, tmp);
                    i += 3;
                    while (i < strlen(inp) && inp[i] != '\0' && inp[i] != '|')
                        i++;
                    left = i;
                }
                else
                {
                    //double pipe
                    if (left) left += 2; 
                    tmp->pipeType = 2;
                    multiPipe(inp, left, i - 1, tmp);
                    i += 2;

                    while (i < strlen(inp) && inp[i] != '\0' && inp[i] != '|')
                        i++;
                    left = i;
                }
            }
            else
            {
                //single pipe
                if (left) left += 1;
                tmp->pipeType = 1;
                multiPipe(inp, left, i - 1, tmp);
                i += 1;

                while (i < strlen(inp) && inp[i] != '\0' && inp[i] != '|')
                    i++;
                left = i;


                
            }
            
            tmp = tmp->next;
            
        }
        else
        {
            i++;
        }
    }

    if (left < i)
    {
        tmp->next = noPipe(inp, left + tmp->pipeType, i - 1);

        if (!head->command[0])
            return tmp->next;
    }

    //printf("No error Here \n");

    return head;

}



