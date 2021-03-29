#include "header.h"

Map *ip_map;

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
    int i = 1;
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

void executor(int clntSocket, Command *command,int input_pipe,int output_pipe) {
    pid_t pid = fork();

    if(pid == 0) {
        int num_bytes;
        char buff[BUFFER_SIZE];
        //INIT IP BUFFER FOR COMMAND
        Input_Buffer * ip_buff = (Input_Buffer *) malloc(sizeof(Input_Buffer));
        ip_buff->flag = 0;
        ip_buff->num_bytes =0;
        ip_buff->end_packet =0;
        strcpy(ip_buff->cmd_buff, command->cmd);

        //INIT OUTPUT BUFFER
        Output_Buffer* op_buff = (Output_Buffer *) malloc(sizeof(Output_Buffer));

        //WRITING COMMAND
        write(clntSocket,ip_buff,sizeof(ip_buff));

        //WRITING INPUT
        while((num_bytes = read(input_pipe,ip_buff->ip_buff,BUFFER_SIZE)) > 0 ) {
            ip_buff -> flag = 1;
            ip_buff -> num_bytes = num_bytes;
            ip_buff -> end_packet =0;
            write(clntSocket,ip_buff,sizeof(Input_Buffer));
        }

        ip_buff->end_packet = 1;
        write(clntSocket,ip_buff,sizeof(Input_Buffer));

        int bytes_read;

        while(1) {
            bytes_read = read(clntSocket,(Output_Buffer *)op_buff,sizeof(Output_Buffer));
            if(op_buff->end_packet) {
                break;
            }
            if(!op_buff->is_error) {
                write(output_pipe,op_buff -> buff,op_buff->num_bytes);
            } else {
                write(2,op_buff -> buff,op_buff->num_bytes);
            }
        }
        exit(0);

    } else {
        if(input_pipe > 0) close(input_pipe);
        wait(NULL);
    }
}


int main(int argc, char const *argv[])
{

    //CONFIG MAP
    ip_map = file_loader(CONFIG_FILE_PATH);

    struct sockaddr_in serv_addr;

    //SOCK INIT
    int clntSocket;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    serv_addr.sin_addr.s_addr = inet_addr(SERV_ADDR);

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

    //CONNECTION ESTABLISHMENT

    if((clntSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Client Socket creation failed");
    }

    int res = connect(clntSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if(res < 0) {
        perror("Connect failed: ");
        exit(0);
    }

    printf("Connection Successful !");


    //COMMAND EXECUTION
    int p_i = 0; 
    Command * current_cmd = cmd_head ->head;
    while (current_cmd!=NULL)
    {
        int input_pipe = p_i-1;
        int output_pipe = p_i;
        // printf("Writing cmd %s to node \n", ip_buff->cmd_buff);
        executor(clntSocket, current_cmd,pipes[input_pipe][0], pipes[output_pipe][1]);
        current_cmd = current_cmd -> next;
    }

    printf("\nPOLO !!!\n");
    return 0;
}
