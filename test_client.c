#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<strings.h>

#define SERV_PORT 1234
#define SERV_ADDR "172.17.74.16"
#define BUFFER_SIZE 20

int main(int argc, char const *argv[])
{
    struct sockaddr_in serv_addr;

    char *buff = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int clntSocket;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    serv_addr.sin_addr.s_addr = inet_addr(SERV_ADDR);

    while(1) {

        if((clntSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Client Socket creation failed");
        }

        int res = connect(clntSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if(res < 0) {
            perror("Connect failed: ");
            exit(0);
        }

        printf("Connection Successful !");

        char cmd[PATH_MAX];

        gets(cmd);
        if(strcmp(cmd,"exit") == 0) {
            close(clntSocket);
            exit(0);
        }
        printf("Writing cmd %s to terminal \n", cmd);

        write(clntSocket, cmd, PATH_MAX);
        int bytes_read;

        while((bytes_read = read(clntSocket,buff,BUFFER_SIZE)) > 0) {
            write(1,buff,bytes_read);
        }

        printf("\nPOLO !!!\n");
    }
    return 0;
}
