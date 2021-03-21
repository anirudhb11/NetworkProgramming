#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>

#define SERV_PORT 1234
#define MAX_PENDING 128
#define BUFFER_SIZE 20


int change_dir(char *abs_p, char *relativ_p) {
    //given old absolute path and new relative path the function returns new
    //absolute path in the absolute path pointer.

    //Returns 0 for success, -1 o.w.

    if(chdir(abs_p) < 0) {
        printf("\nError in accessing old path. Exiting");
        return -1;
    }
    if(chdir(relativ_p) < 0) {
        printf("\nThe change is not valid");
        //What to do in this case ? Return the error back to requesting function.
        return -1;
    }
    if (getcwd(abs_p, sizeof(abs_p)) != NULL) {
       printf("\nCurrent working dir: %s\n", abs_p);
       return -1;
    } 
    else {
       perror("\ngetcwd() error");
       return -1;
    }

    return 0;
}

void executor(char * cmd, char * directory, int clntSocket) {

    char *param = strtok(cmd, " ");

    if (!strcmp(param,"cd")) {
        int i =0;
        while(param[i] != '\0') i++;
        param += i+1;
        change_dir(directory,param);
    }
    else {
        //assuming the total number of parameters cannot be greater than max length of shell command
        char **args = (char **) calloc(PATH_MAX, sizeof(char *));
        args[0] = param;
        int arg_length = 0, i =1;
        while(param != NULL) {
            param= strtok(NULL, " ");
            args[i] = param;
            i++;
        }
        arg_length = i-1;

        int p[2];
        char *buff = (char *) malloc(BUFFER_SIZE * sizeof(char));
        pipe(p);
        
        pid_t exec_proc = fork();
        if(exec_proc == 0) {
            //Closing read end of pipe
            close(p[0]);
            close(1);
            dup(p[1]);
            int ch_out = chdir(directory);
            if(ch_out < 0) {
                perror("Change directory error: ");
                exit(1);
            }
            if(execvp(args[0],args) < 0) {
                // close(1);
                // dup2(1);
                printf("\nCoudn't execute command\n");
                perror("Execution error: ");
                exit(1);
            }
        } else {
            //Closing write end of pipe.
            close(p[1]);
            int num_bytes;
            while((num_bytes = read(p[0],buff,BUFFER_SIZE)) > 0 ) {
                printf("%d\n",num_bytes);
                write(clntSocket,buff,num_bytes);
            }
            wait(NULL);
            return;
        }

    }
    

}

int main(int argc, char const *argv[])
{
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

        printf("\nHandling client: %s",inet_ntoa(clnt_addr.sin_addr));

        int ret = fork();
        if(ret == 0) {
            //Child Process particular to a single node client.

            char cmd_buffer[PATH_MAX];
            char c;
            close(servSocket);
            if(read(clntSocket, cmd_buffer, PATH_MAX) == 0) {
                printf("\nProcess ended by client\n");
                exit(0);
            }
            printf("\nThe command is: %s and serving process id is: %d\n", cmd_buffer, getpid());
            executor(cmd_buffer,".",clntSocket);
            exit(0);
        }
        else {
            close(clntSocket);
        }
    }

    return 0;
}
