#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define SERV_PORT 1234
#define MAX_PENDING 128


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

int main(int argc, char const *argv[])
{
    int servSocket, clntSocket;
    struct sockaddr_in serv_addr, clnt_addr;
    if((servSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("\nServer Socket Creation Failed\n");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if(bind(servSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nServer bind failed\n");
    }

    printf("1 reached");

    if(listen(servSocket, MAX_PENDING) < 0) {
        printf("\nServer listen failed\n");
    }

    for(;;) {
        socklen_t clntLen = sizeof(clnt_addr);
        printf("2 reached");
        if((clntSocket = accept(servSocket,(struct sockaddr*) &clnt_addr, &clntLen)) <0 ) {
            printf("\nAccept connection failed on incoming connection");
        }

        printf("\nHandling client: %s",inet_ntoa(clnt_addr.sin_addr));

        int ret = fork();
        if(ret == 0) {
            //Child Process particular to a single node client.

            char cmd_buffer[PATH_MAX];
            char c;
            close(servSocket);
            while(1) {
                if(read(clntSocket, cmd_buffer, PATH_MAX) == 0) {
                    printf("sf");
                    exit(0);
                }
                printf("The command is: %s and process id is: %d\n", cmd_buffer, getpid());
                fflush(stdout);
                // scanf("%c",&c);
            }
            // 

        }
    }

    return 0;
}
