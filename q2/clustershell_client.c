#include "header.h"

Map *ip_map;

int NODE_COUNT;

Command * fill_command(char *param) {
    Command * command = (Command *) malloc(sizeof(Command));

    command->node = NULL;
    command->cmd = param;
    command->next = NULL;
    return command;
}

void process_command(Command * cmd) {
    char *node = ltrim(rtrim(strtok(cmd->cmd, "."),' '), ' ');
    int i =0;
    while(cmd->cmd[i] != '\0'){
        i++;
    }
    while (cmd->cmd[i] == '\0')
    {
        i++;
    }
    
    cmd->cmd += i;
    char *m_cmd = ltrim(rtrim(cmd->cmd,' '), ' ');
    cmd->node = node;
    cmd->cmd = m_cmd;
}

Command_List* parse_command(char *cmd) {
    Command_List* command_head = (Command_List *) malloc(sizeof(Command_List));

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

void executor(Command *command,int* input_pipe,int* output_pipe) {
    pid_t pid = fork();

    if(pid == 0) {
        int num_bytes;
        char buff[BUFFER_SIZE];

        //SOCK INIT
        struct sockaddr_in serv_addr;
        int clntSocket;
        bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERV_PORT);
        
        int node;

        if((node = find_map_node(command->node, ip_map, NODE_COUNT)) == -1) {
            printf("\nNode %s does not exist\n",command->node);
            exit(1);
        }
        serv_addr.sin_addr.s_addr = inet_addr(ip_map[node].ip);

        //INIT IP BUFFER FOR COMMAND
        Input_Buffer * ip_buff = (Input_Buffer *) malloc(sizeof(Input_Buffer));
        ip_buff->flag = 0;
        ip_buff->num_bytes =0;
        ip_buff->end_packet =0;
        strcpy(ip_buff->cmd_buff, command->cmd);

        //INIT OUTPUT BUFFER
        Output_Buffer* op_buff = (Output_Buffer *) malloc(sizeof(Output_Buffer));

        //CONNECTION ESTABLISHMENT

        if((clntSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Client Socket creation failed\n");
            exit(1);
        }

        int res = connect(clntSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if(res < 0) {
            perror("Connect failed\n");
            exit(1);
        }

        // printf("\nConnection Successful to server %s !\n",ip_map[find_map_node(command->node, ip_map)].ip);

        //WRITING COMMAND
        write(clntSocket,ip_buff,sizeof(Input_Buffer));

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
            write(clntSocket,ip_buff,sizeof(Input_Buffer));
        }

        int bytes_read;
        while(1) {
            bytes_read = read(clntSocket,(Output_Buffer *)op_buff,sizeof(Output_Buffer));
            if(op_buff->end_packet) {
                break;
            }
            if(!op_buff->is_error) {
                if(output_pipe !=NULL) {
                    write(output_pipe[1],op_buff -> buff,op_buff->num_bytes);
                }
                else{
                    write(1,op_buff->buff,op_buff->num_bytes);
                } 
            } else {
                write(2 ,op_buff -> buff,op_buff->num_bytes);
            }
        }
        exit(0);

    } else {
        wait(NULL);
    }
}


int main(int argc, char const *argv[])
{

    //CONFIG MAP
    ip_map = file_loader(CONFIG_FILE_PATH, &NODE_COUNT);

    //COMMAND/ IO BUFFER INIT AND PROCESS

    while (1)
    {
        pid_t pid = fork();

        if(pid == 0) {
            char cmd[PATH_MAX];
            printf("\n$ ");
            gets(cmd);

            if(!strcmp(cmd,"nodes")) {
                for(int i= 0 ; i < NODE_COUNT ; i++ ){
                    printf("Node :%s \t IP %s\n",ip_map[i].node, ip_map[i].ip);
                }
                exit(0);
            }

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
                int *input_pipe = p_i - 1 >=0 ? pipes[p_i - 1] : NULL;
                int *output_pipe = (p_i == cmd_len -1) ? NULL : pipes[p_i];

                int node;
                if((node = find_map_node(current_cmd->node, ip_map, NODE_COUNT)) == -2) {

                    for (int i = 0; i < NODE_COUNT; i++)
                    {
                        strcpy(current_cmd->node, ip_map[i].node);
                        executor(current_cmd, NULL, output_pipe);
                    }
                    
                } else {
                    executor(current_cmd,input_pipe, output_pipe);
                }
                if(output_pipe != NULL) {
                    close(output_pipe[1]);
                }
                current_cmd = current_cmd -> next;
                p_i++;

            }
        } else {
            wait(NULL);
            printf("\n\n");
        }

    }
    return 0;
}
