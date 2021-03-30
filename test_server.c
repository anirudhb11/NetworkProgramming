#include "header.h"

char ** directories;

int change_dir(int node, char *relativ_p, int pipe_fd, int clntSocket){
    //given old absolute path and new relative path the function returns new
    //absolute path in the absolute path pointer.

    //Returns 0 for success, -1 o.w.

    int p_err[2];
    pipe(p_err);
    int temp;

    // printf("\nThe PID is: %d\n", getpid());
    // printf("\nThe write end is: %d and read end is: %d\n",p_err[1], p_err[0]);
    close(2);
    dup(p_err[1]);
    close(p_err[1]);

    if(chdir(directories[node]) < 0) {
        printf("\nCurrent stored directory path for node %d has been corrupted\n", node);
        return -1;
    }
    if(chdir(relativ_p) < 0) {
        perror("\nThe change is not valid: ");
        // printf("The value is: %d", p_err[1]);
        close(2);
        //printf("\nThe change is not valid");
        //What to do in this case ? Return the error back to requesting function.
        Output_Buffer * buff = (Output_Buffer *) malloc(sizeof(Output_Buffer));
        int num_bytes;
        while((num_bytes = read(p_err[0],buff->buff,BUFFER_SIZE)) > 0 ) {
                        //printf("\nError:%s\n",buff->buff);
                        buff -> is_error = 1;
                        buff -> num_bytes = num_bytes;
                        buff -> end_packet = 0;
                        write(clntSocket,buff,sizeof(Output_Buffer));
                        printf("\nStat2: %s\n", buff->buff);
                    }
        buff->end_packet = 1;
        buff->num_bytes= 0;
        write(clntSocket,buff,sizeof(Output_Buffer));
        
        close(p_err[0]);
        return -1;
    }

    char direcbuff[PATH_MAX];
    if (getcwd(direcbuff, PATH_MAX) != NULL) {
       printf("\nNew CWD for node %d is : %s\n",node, direcbuff);
       write(pipe_fd, direcbuff, strlen(direcbuff));
       return 0; 
    } 
    else {
       perror("\ngetcwd() error");
       return -1;
    }

    return 0;
}

void executor(char * cmd, int node, int clntSocket, int pipe_fd) {

    char *param = strtok(cmd, " ");

    if (!strcmp(param,"cd")) {
        int i =0;
        while(param[i] != '\0') i++;
        param += i+1;
        change_dir(node, param, pipe_fd, clntSocket);
        return;
    }
    else {
        //assuming the total number of parameters cannot be greater than max length of shell command
        char **args = (char **) calloc(PATH_MAX, sizeof(char *));
        int p[2];
        int p_err[2];
        pipe(p);
        pipe(p_err);
        args[0] = param;
        int arg_length = 0, i =1;
        while(param != NULL) {
            param= strtok(NULL, " ");
            args[i] = param;
            i++;
        }
        arg_length = i-1;
        // char *buff = (char *) malloc(BUFFER_SIZE * sizeof(char));

        Output_Buffer * buff = (Output_Buffer*) malloc(sizeof(Output_Buffer));
        
        pid_t exec_proc = fork();

        int input_pipe[2];
        pipe(input_pipe);

        if(exec_proc == 0) {
            //Closing read end of pipe
            close(p[0]);
            close(p_err[0]);
            close(input_pipe[1]);

            close(1);
            dup(p[1]);
            close(2);
            dup(p_err[1]);

            close(0);
            dup(input_pipe[0]);
            int ch_out = chdir(directories[node]);
            if(ch_out < 0) {
                perror("\nChange directory error: ");
                exit(1);
            }

            printf("C1\n");
            if(execvp(args[0],args) < 0) {
                perror("\nExecution Error: ");
                exit(1);
            }
        } else {
            //Closing write end of pipes.
            close(p[1]);
            close(p_err[1]);
            //Closing read end of input pipe.
            close(input_pipe[0]);

            printf("P1\n");

            int stat, num_bytes;
            Input_Buffer* ip_buff = (Input_Buffer *) malloc(sizeof(Input_Buffer));

            while (1)
            {
                read(clntSocket, ip_buff, sizeof(Input_Buffer));
                printf("The read packet is: %s :: %d \n\n", ip_buff->ip_buff, ip_buff->end_packet);
                if(ip_buff->end_packet) break;

                write(input_pipe[1],ip_buff->ip_buff,ip_buff->num_bytes);
            }

            printf("P2\n");

            close(input_pipe[1]);
            

            wait(&stat);
            printf("\nStat is: %d\n",stat);
            if(WIFEXITED(stat)) {
                if(WEXITSTATUS(stat) > 0) {
                    while((num_bytes = read(p_err[0],buff->buff,BUFFER_SIZE)) > 0 ) {
                        //printf("\nError:%s\n",buff->buff);
                        buff -> is_error = 1;
                        buff -> num_bytes = num_bytes;
                        buff -> end_packet = 0;
                        write(clntSocket,buff,sizeof(Output_Buffer));
                    }

                }
                else {
                    while((num_bytes = read(p[0],buff->buff,BUFFER_SIZE)) > 0 ) {
                        //printf("\nNormal:%s\n",buff->buff);
                        buff -> is_error = 0;
                        buff -> num_bytes = num_bytes;
                        buff -> end_packet = 0;
                        write(clntSocket,buff,sizeof(Output_Buffer));
                    }
                }

                buff->end_packet = 1;
                buff->num_bytes= 0;
                write(clntSocket,buff,sizeof(Output_Buffer));
            }
            return;
        }

    }
}

int main(int argc, char const *argv[])
{
    // Loading ips' map into the system.
    Map * ip_map = file_loader(CONFIG_FILE_PATH);

    directories = (char **) malloc(NODE_COUNT * sizeof(char *));
    for (int i = 0; i < NODE_COUNT; i++)
    {
        directories[i] = (char*) malloc(sizeof(char) * PATH_MAX);
        strcpy(directories[i], HOME_DIRECTORY);
    }
    

    
    int servSocket, clntSocket;
    struct sockaddr_in serv_addr, clnt_addr;
    if((servSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("\nServer Socket Creation Failed\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if(bind(servSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nServer bind failed\n");
        exit(1);
    }

    if(listen(servSocket, MAX_PENDING) < 0) {
        printf("\nServer listen failed\n");
        exit(1);
    }

    for(;;) {
        socklen_t clntLen = sizeof(clnt_addr);

        if((clntSocket = accept(servSocket,(struct sockaddr*) &clnt_addr, &clntLen)) <0 ) {
            printf("\nAccept connection failed on incoming connection");
            continue;
        }

        printf("\n\nHandling client: %s\n",inet_ntoa(clnt_addr.sin_addr));
        int node;
        if((node = find_map_ip(inet_ntoa(clnt_addr.sin_addr), ip_map)) < 0) {
            printf("\nClient not in Config File List\n");
            exit(1);
        }

        int direc_pipe[2];
        pipe(direc_pipe);

        // printf("\nThe direc pipe write end is: %d and read end is %d\n", direc_pipe[1], direc_pipe[0]);

        int ret = fork();
        if(ret == 0) {
            //Child Process particular to a single node client.

            //char cmd_buffer[PATH_MAX];
            Input_Buffer * ip_buff = (Input_Buffer *) malloc(sizeof(Input_Buffer));
            char c;
            close(servSocket);
            close(direc_pipe[0]);
            if(read(clntSocket, ip_buff, sizeof(Input_Buffer)) == 0) {
                printf("\nProcess ended by client\n");
                exit(0);
            }
            printf("\nThe command is: %s and serving process id is: %d\n", ip_buff->cmd_buff, getpid());
            executor(ip_buff->cmd_buff,node,clntSocket, direc_pipe[1]);
            exit(0);
        }
        else {
            close(clntSocket);
            close(direc_pipe[1]);
            int count_bytes = read(direc_pipe[0],directories[node],PATH_MAX);
            if(count_bytes>0 && count_bytes<PATH_MAX) 
                directories[node][count_bytes] = '\0';
            printf("\nThe changed directory is: %s\n", directories[node]);
            wait(NULL);
            close(direc_pipe[0]);
        }
    }

    return 0;
}
