#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<limits.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<strings.h>

#define SERV_PORT 1235
#define SERV_ADDR "172.17.74.15"
#define BUFFER_SIZE 20

typedef struct Buffer {
    int is_error;
    int num_bytes;
    char buff[BUFFER_SIZE];
} Buffer;

int main(int argc, char const *argv[])
{
    struct sockaddr_in serv_addr;

    // char *buff = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int clntSocket;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    serv_addr.sin_addr.s_addr = inet_addr(SERV_ADDR);

    Buffer* buff = (Buffer *) malloc(sizeof(Buffer));


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

        while((bytes_read = read(clntSocket,(Buffer *)buff,sizeof(Buffer))) > 0) {
            if(!buff->is_error) {
                write(1,buff -> buff,buff->num_bytes);
            } else {
                write(2,buff -> buff,buff->num_bytes);
            }
        }

        printf("\nPOLO !!!\n");
    }
    return 0;
}
