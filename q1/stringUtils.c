#include "header.h"
bool search(char *inp, char ch)
{
    int i = 0;
    while (inp[i] != '\0')
    {
        if (inp[i] == ch)
        {
            inp[i] = '\0';
            return true;
        }
        i++;
    }
    return false;
}
int charPos(char *inp, char ch)
{
    int i = 0;
    while (inp[i] != '\0')
    {
        if (inp[i] == ch)
        {
            inp[i] = '\0';
            return i;
        }
        i++;
    }
    return -1;
}

char *slicestring(int left, int right, char *inp)
{
    char *s = (char *)malloc((right - left + 1) * sizeof(char));
    int j = 0;
    //printf("slicing from %d to %d \n", left, right);

    for (int i = left; i <= right; i++)
    {
        s[j++] = inp[i];
    }

    return s;
}
char *trimwhitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}
char **tokenize(char *line)
{
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, WHITESPACE_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;
        if (position >= bufsize)
        {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, WHITESPACE_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}
