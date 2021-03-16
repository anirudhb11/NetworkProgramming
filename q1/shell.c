#include "shell.h"


void command_initialization(command *cmd){
    for(int cmd_index = 0; cmd_index < MAX_TO_PIPE_COMMANDS; cmd_index++){
        cmd->com_name[cmd_index] = NULL;
        for(int arg_index = 0; arg_index < MAX_ARGS; arg_index++){
            cmd->argv[cmd_index][arg_index] = NULL;
        }
        cmd->num_args[cmd_index] = 0;
        cmd->input_redirected[cmd_index] = false;
        cmd->output_redirected[cmd_index] = false;
        cmd->output_append[cmd_index] = false;

        cmd->input_file[cmd_index] = NULL;
        cmd->output_file[cmd_index] = NULL;
    }
}

int main(){
    command c1;
    command_initialization(&c1);
    //To run ls -a
    c1.com_name[0] = (char *)malloc(sizeof(char) * MAX_ARG_LEN);
    c1.num_args[0] = 2;
    for(int arg_index=0;arg_index < c1.num_args[0];arg_index++){
        c1.argv[0][arg_index] = (char *)malloc(sizeof(char) * MAX_ARG_LEN);
    }


    strcpy(c1.com_name[0], "/bin/ls");
    strcpy(c1.argv[0][0], "ls");
    strcpy(c1.argv[0][1], "-a");

    //Going to execute ls
    int ret = fork();
    if(ret == 0){
        printf("Command Name: %s\n", c1.com_name[0]);
        printf("First arg: %s\n", c1.argv[0][0]);
        printf("Second arg: %s\n", c1.argv[0][1]);
        int ret = execv(c1.com_name[0], c1.argv[0]);
        if(ret == -1){
            perror("Exec call didn't succeed ");
        }
    }
    else{
        wait(NULL);
        printf("Command has been executed\n");
    }

}