#include "header.h"

int pipeCount(commandGroup* cmd)
{
    commandGroup *tmp = cmd;
    int count = 0;
    while(tmp)
    {
        tmp = tmp->next;
        count++;
    }
    return count;
}

int execCommand(commandGroup* cmd)
{
    int pipeNum = 3;//pipeCount(cmd);
    printf("There are %d pipes\n" , pipeNum);

    
    /*
    int pipeArray[pipeNum][2];
    for(int i = 0 ; i < pipeNum; i++)
    {
        pipe(pipeArray[i]);
        printf("Pipe FDs of pipe:%d -> Read End:%d, Write End:%d\n", i, pipeArray[i][0], pipeArray[i][1]);
    }q
    */

    int fd[pipeNum][2];
    int status;
    int isFirstPipe = 1;
    int pid[pipeNum];
    int count = 0;
    int i;

    commandGroup *tmp = cmd;
    
    int y = -1;
    while (y++ < 1)
    {
        status = pipe(fd[count]);
        if (status < 0)
        {
            printf("\npipe error");
            return -1;
        }

        pid[count] = fork();

        if (pid[count] < 0)
        {
            printf("\nError in fork");
            return -1;
        }
        else if (pid[count]) /*father code*/
        {
            if (cmd->next)
            {
                cmd = cmd->next;
                count++;
                continue;
            }

            //close all opened pipes
            for (i = 0; i <= count; i++)
            {
                close(fd[i][0]);
                close(fd[i][1]);
            }

            for(int i = 0 ;i <= count; i++)
                waitpid(pid[i], NULL,  0);
            
            return 0;
        }
        else /*son code*/
        {
            if (count == 0)
                close(fd[0][0]);
            else
                dup2(fd[count - 1][0], 0);

            if (!cmd->next)
                close(fd[count][1]);
            else
                dup2(fd[count][1], 1);

            //close all other opened pipes
            for (i = 0; i <= count; i++)
            {
                close(fd[i][0]);
                close(fd[i][1]);
            }

            execv(cmd->command[0], cmd->argv[0]);

            //if execv failed
            printf("%s: failed to execute", cmd->argv[0][0]);
            return -1;
        }
    }

    return -1;
}

