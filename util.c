#include "header.h"

int find_map(char *ip, Map* ip_map) {
    for(int i=0 ; i < NODE_COUNT ; i++) {
        if(!strcmp(ip_map[i].ip, ip)) {
            return i;
        }
    }
    return -1;
}

Map* file_loader(char *config_file_path) {
    //Assuming the ip mapping is stored in config.txt
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    Map * ip_map = (Map *) malloc(NODE_COUNT * sizeof(Map));

    fp = fopen(config_file_path, "r");
    if(fp == NULL) {
        printf("\nConfig file loading error\n");
        exit(1);
    }

    int count = 0;

    while ((read = getline(&line,&len, fp)) != -1)
    {
        if(count >= NODE_COUNT) {
            printf("\nBad config file. More IPs than configuration\n");
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

    fclose(fp);
    return ip_map;
}