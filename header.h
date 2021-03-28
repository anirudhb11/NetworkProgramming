#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>

#define SERV_PORT 1235
#define MAX_PENDING 128
#define BUFFER_SIZE 20
#define CONFIG_FILE_PATH "./config.txt"
#define NODE_COUNT 3
#define HOME_DIRECTORY "/Users/ashishkumar"
#define SERV_ADDR "172.17.32.24"

typedef struct Map {
    char* node;
    char* ip;
} Map;

typedef struct Buffer {
    int is_error;
    int num_bytes;
    char buff[BUFFER_SIZE];
} Buffer;

typedef struct Command {
    char *node;
    char *cmd;
} Command;

int find_map(char *ip, Map* ip_map);
Map* file_loader(char *config_file_path);