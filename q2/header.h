#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<assert.h>

#define SERV_PORT 1235
#define MAX_PENDING 128
#define BUFFER_SIZE 20
#define CONFIG_FILE_PATH "./config.txt"
#define NODE_COUNT 3
#define HOME_DIRECTORY "/Users/"
#define SERV_ADDR "172.17.32.24"

typedef struct Map {
    char* node;
    char* ip;
} Map;

typedef struct Output_Buffer {
    int is_error;
    int num_bytes;
    int end_packet;
    char buff[BUFFER_SIZE];
} Output_Buffer;

typedef struct Input_Buffer {
    int flag; // 0- Command buffer , 1 -Input buffer
    char cmd_buff[PATH_MAX];
    int num_bytes;
    int end_packet;
    char ip_buff[BUFFER_SIZE];
} Input_Buffer;

typedef struct Command {
    char *node;
    char *cmd;
    struct Command *next;
} Command;

typedef struct Command_List {
    Command * head;
    int size;
} Command_List;

int find_map_ip(char *ip, Map* ip_map);
int find_map_node(char *node, Map* ip_map);
Map* file_loader(char *config_file_path);
char* rtrim(char* string, char junk);
char* ltrim(char* string, char junk);