#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<strings.h>

#define SERV_PORT 1234
#define SERV_ADDR "172.17.74.14"

int main(int argc, char const *argv[])
{
    struct sockaddr_in serv_addr;
    int clntSocket;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    serv_addr.sin_addr.s_addr = inet_addr(SERV_ADDR);

    if((clntSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Client Socket creation failed");
    } 

    int res = connect(clntSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if(res < 0) {
        printf("Connect failed");
    }

    char cmd[PATH_MAX];
    while(1) {

        scanf("%s",cmd);
        if(strcmp(cmd,"exit") == 0) {
            close(clntSocket);
            exit(0);
        }

        write(clntSocket, cmd, PATH_MAX);
    }
    return 0;
}
