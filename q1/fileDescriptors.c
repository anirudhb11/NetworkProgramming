#include "header.h"

int *getDescriptors(char *command, commandGroup* cmd)
{
    int *fds = (int *)malloc(2 * sizeof(int));
    char *inputfile = (char *)malloc(20);
    char *outputfile = (char *)malloc(20);
    //Default file numbers
    fds[0] = STDIN_FILENO;
    fds[1] = STDOUT_FILENO;
    int i = 0;
    int j = 0;
    while (command[i] != '\0' && command[i] != '|')
    {
        if (command[i] == '>')
        {
            command[i - 1] = '\0';
            j = 0;
            if (command[i + 1] == '>')
            {
                if (command[i + 2] == ' ')
                {
                    i += 2;
                    while (command[i] == ' ')
                        i++;
                    while (command[i] != '\0' && command[i] != ' ')
                        outputfile[j++] = command[i++];
                    
                }
                else
                {
                    i += 2;
                    while (command[i] != '\0' && command[i] != ' ')
                    {
                        outputfile[j++] = command[i++];
                    }
                    int outputfd = open(outputfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                    fds[1] = outputfd;
                }
            }
            else if (command[i + 1] == ' ')
            {
                //single >
                i += 2;
                while (command[i] != '\0' && command[i] != ' ')
                {
                    outputfile[j++] = command[i++];
                }
                int outputfd = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                fds[1] = outputfd;
            }
            else
            {
                i += 1;
                while (command[i] != '\0' && command[i] != ' ')
                {
                    outputfile[j++] = command[i++];
                }
                int outputfd = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                fds[1] = outputfd;
            }
        }
        else if (command[i] == '<')
        {
            command[i - 1] = '\0';
            j = 0;
            if (command[i + 1] == ' ')
            {
                //single <
                i += 2;
                while (command[i] != '\0' && command[i] != ' ')
                {
                    inputfile[j++] = command[i++];
                }
                int inputfd = open(inputfile, O_RDONLY);
                fds[0] = inputfd;
            }
            else
            {
                i += 1;
                while (command[i] != '\0' && command[i] != ' ')
                {
                    inputfile[j++] = command[i++];
                }
                int inputfd = open(inputfile, O_RDONLY);
                fds[0] = inputfd;
            }
        }
        i++;
    }
    return fds;
}


