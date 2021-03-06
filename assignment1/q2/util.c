#include "header.h"

int find_map_ip(char *ip, Map* ip_map, int NODE_COUNT) {
    for(int i=0 ; i < NODE_COUNT ; i++) {
        if(!strcmp(ip_map[i].ip, ip)) {
            return i;
        }
    }
    return -1;
}

int find_map_node(char *node, Map*ip_map, int NODE_COUNT) {
    if (!strcmp("n*",node)) {
        return -2;
    }
    for(int i=0 ; i < NODE_COUNT ; i++) {
        if(!strcmp(ip_map[i].node, node)) {
            return i;
        }
    }
    return -1;
}

Map* file_loader(char *config_file_path, int * node_len) {
    //Assuming the ip mapping is stored in config.txt
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    Map * ip_map = (Map *) malloc(MAX_NODE_COUNT * sizeof(Map));

    fp = fopen(config_file_path, "r");
    if(fp == NULL) {
        printf("\nConfig file loading error\n");
        exit(1);
    }

    int count = 0;

    while ((read = getline(&line,&len, fp)) != -1)
    {
        if(count >= MAX_NODE_COUNT) {
            printf("\nBad config file. More nodes than MAX_NODE_COUNT\n");
            exit(1);
        }
        char * temp = (char *) malloc(sizeof(char) * len);
        strcpy(temp,line);
        char *node = strtok(temp, " \n");
        char *ip = strtok(NULL, " \n");
        ip_map[count].node = node;
        ip_map[count].ip = ip;

        count++;
    }

    *node_len = count;
    fclose(fp);
    return ip_map;
}

char* rtrim(char* string, char junk)
{
    char* original = string + strlen(string);
    while(*--original == junk);
    *(original + 1) = '\0';
    return string;
}

char* ltrim(char *string, char junk)
{
    char* original = string;
    char *p = original;
    int trimmed = 0;
    do
    {
        if (*original != junk || trimmed)
        {
            trimmed = 1;
            *p++ = *original;
        }
    }
    while (*original++ != '\0');
    return string;
}