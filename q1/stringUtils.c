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
char **tokenize(char *line, char* delim)
{
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, delim);
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

        token = strtok(NULL, delim);
    }

    tokens[position] = NULL;
    return tokens;
}
char *findPath(char *token0)
{
    token0 = trimwhitespace(token0);
    const char *path = getenv("PATH");
    char *buf;
    char buf3[400];
    strcpy(buf3, path);
    char *buf2 = (char *)malloc(50 * sizeof(char));
    int i = 0;
    int left = 0;
    while (buf3[i] != '\0')
    {
        if (buf3[i] == ':')
        {
            buf = slicestring(left, i - 1, buf3);
            left = i + 1;
            strcpy(buf2, buf);
            buf2 = strcat(buf2, "/");
            buf2 = strcat(buf2, token0);
            if (access(buf2, X_OK) == 0)
            {
                return buf2;
            }
        }
        i++;
    }
    buf = slicestring(left, i - 1, buf3);
    strcpy(buf2, buf);
    buf2 = strcat(buf2, "/");
    buf2 = strcat(buf2, token0);
    if (access(buf2, X_OK) == 0)
    {
        return buf2;
    }
    return NULL;
}
