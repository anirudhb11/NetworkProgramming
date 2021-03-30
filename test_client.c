#include "header.h"

Map *ip_map;


// int find_map_ip(char *ip, Map* ip_map) {
//     for(int i=0 ; i < NODE_COUNT ; i++) {
//         if(!strcmp(ip_map[i].ip, ip)) {
//             return i;
//         }
//     }
//     return -1;
// }

// int find_map_node(char *node, Map*ip_map) {
//     for(int i=0 ; i < NODE_COUNT ; i++) {
//         if(!strcmp(ip_map[i].node, node)) {
//             return i;
//         }
//     }
//     return -1;
// }

// Map* file_loader(char *config_file_path) {
//     //Assuming the ip mapping is stored in config.txt
//     FILE * fp;
//     char * line = NULL;
//     size_t len = 0;
//     ssize_t read;

//     Map * ip_map = (Map *) malloc(NODE_COUNT * sizeof(Map));

//     fp = fopen(config_file_path, "r");
//     if(fp == NULL) {
//         printf("\nConfig file loading error\n");
//         exit(1);
//     }

//     int count = 0;

//     while ((read = getline(&line,&len, fp)) != -1)
//     {
//         if(count >= NODE_COUNT) {
//             printf("\nBad config file. More IPs than configuration\n");
//             exit(1);
//         }
//         char * temp = (char *) malloc(sizeof(char) * len);
//         strcpy(temp,line);
//         char *node = strtok(temp, " \n");
//         char *ip = strtok(NULL, " \n");
//         ip_map[count].node = node;
//         ip_map[count].ip = ip;

//         count++;
//     }

//     fclose(fp);
//     return ip_map;
// }

// char* rtrim(char* string, char junk)
// {
//     char* original = string + strlen(string);
//     while(*--original == junk);
//     *(original + 1) = '\0';
//     return string;
// }

// char* ltrim(char *string, char junk)
// {
//     char* original = string;
//     char *p = original;
//     int trimmed = 0;
//     do
//     {
//         if (*original != junk || trimmed)
//         {
//             trimmed = 1;
//             *p++ = *original;
//         }
//     }
//     while (*original++ != '\0');
//     return string;
// }

Command * fill_command(char *param) {
    Command * command = (Command *) malloc(sizeof(Command));

    command->node = NULL;
    command->cmd = param;
    command->next = NULL;
    return command;
}

void process_command(Command * cmd) {
    char *node = ltrim(rtrim(strtok(cmd->cmd, "."),' '), ' ');
    char *m_cmd = ltrim(rtrim(strtok(NULL,"."),' '), ' ');
    cmd->node = node;
    cmd->cmd = m_cmd;
}


Command_List* parse_command(char *cmd) {
    Command_List* command_head = (Command_List *) malloc(sizeof(Command_List));
    // command_head = NULL;
    int i = 0;
    char *param = strtok(cmd, "|");
    Command * command, *current_cmd;
    while(param != NULL) {
        command = fill_command(param);
        if(command_head->head == NULL) {
            command_head->head = command;
            current_cmd = command;
        } else {
            current_cmd -> next = command;
            current_cmd = command;
        }
        param = strtok(NULL, "|");
        i++;
    }
    command_head->size = i;
    current_cmd = command_head->head;
    while (current_cmd!=NULL)
    {
        process_command(current_cmd);
        current_cmd = current_cmd -> next;
    }
    return command_head;
}

// int main(int argc, char const *argv[])
// {
//     char cmd[PATH_MAX];
//     strcpy(cmd,"n1.ls |   n2.tr | n3.rt#2 f1m");
//     Command ** t = parse_command(cmd);
//     printf("YOLO");
//     return 0;
// }

void executor(Command *command,int* input_pipe,int* output_pipe) {
    pid_t pid = fork();

    if(pid == 0) {
        int num_bytes;
        char buff[BUFFER_SIZE];

        //SOCK INIT
        printf("P1\n");
        struct sockaddr_in serv_addr;
        int clntSocket;
        bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERV_PORT);

        serv_addr.sin_addr.s_addr = inet_addr(ip_map[find_map_node(command->node, ip_map)].ip);

        //INIT IP BUFFER FOR COMMAND
        printf("P2\n");
        Input_Buffer * ip_buff = (Input_Buffer *) malloc(sizeof(Input_Buffer));
        ip_buff->flag = 0;
        ip_buff->num_bytes =0;
        ip_buff->end_packet =0;
        strcpy(ip_buff->cmd_buff, command->cmd);

        //INIT OUTPUT BUFFER
        Output_Buffer* op_buff = (Output_Buffer *) malloc(sizeof(Output_Buffer));

        //CONNECTION ESTABLISHMENT

        if((clntSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Client Socket creation failed");
        }

        int res = connect(clntSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if(res < 0) {
            perror("Connect failed: ");
            exit(0);
        }

        printf("\nConnection Successful !\n");
        printf("bkkbkbk\n");

        // printf("\nThe input pipe is: %d\n", input_pipe[0] );

        //WRITING COMMAND
        write(clntSocket,ip_buff,sizeof(ip_buff));

        //WRITING INPUT IN CASE INPUT IS THERE.
        if(input_pipe != NULL) {
            while((num_bytes = read(input_pipe[0],ip_buff->ip_buff,BUFFER_SIZE)) > 0 ) {
                ip_buff -> flag = 1;
                ip_buff -> num_bytes = num_bytes;
                ip_buff -> end_packet =0;
                write(clntSocket,ip_buff,sizeof(Input_Buffer));
            }

            ip_buff->end_packet = 1;
            write(clntSocket,ip_buff,sizeof(Input_Buffer));
        } else {

            ip_buff->end_packet = 1;
            strcpy(ip_buff->ip_buff, "sad");

            printf("The string is %s and %d\n",ip_buff->ip_buff, ip_buff->end_packet);
            printf("The value is %d\n", write(clntSocket,ip_buff,sizeof(Input_Buffer)));

            printf("P3\n");
        }

        int bytes_read;

        while(1) {
            bytes_read = read(clntSocket,(Output_Buffer *)op_buff,sizeof(Output_Buffer));
            if(op_buff->end_packet) {
                break;
            }
            if(!op_buff->is_error) {
                if(output_pipe !=NULL) write(output_pipe[1],op_buff -> buff,op_buff->num_bytes);
                else write(1,op_buff->buff,op_buff->num_bytes);
            } else {
                write(2 ,op_buff -> buff,op_buff->num_bytes);
            }
        }
        exit(0);

    } else {
        if(input_pipe != NULL) close(input_pipe[1]);
        wait(NULL);
    }
}


int main(int argc, char const *argv[])
{

    //CONFIG MAP
    ip_map = file_loader(CONFIG_FILE_PATH);

    //COMMAND/ IO BUFFER INIT AND PROCESS

    char cmd[PATH_MAX];

    gets(cmd);
    Command_List *cmd_head = parse_command(cmd);

    int cmd_len = cmd_head->size;

    //PIPES INIT
    int pipes[cmd_len -1][2];

    for(int i=0; i< cmd_len - 1 ; i++) {
        pipe(pipes[i]);
    }


    //COMMAND EXECUTION
    int p_i = 0; 
    Command * current_cmd = cmd_head ->head;
    while (current_cmd!=NULL)
    {
        printf("pi:%d\n",p_i);
        assert(cmd_len == 1);
        int *input_pipe = p_i - 1 >=0 ? pipes[p_i - 1] : NULL;
        int *output_pipe = (p_i == cmd_len -1) ? NULL : pipes[p_i];

        printf("P0\n");
        // printf("Writing cmd %s to node \n", ip_buff->cmd_buff);
        assert(input_pipe == NULL);
        assert(output_pipe == NULL);
        executor(current_cmd,input_pipe, output_pipe);
        current_cmd = current_cmd -> next;
    }

    printf("\nPOLO !!!\n");
    return 0;
}
