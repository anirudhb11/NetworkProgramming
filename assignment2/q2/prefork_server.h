#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <string.h>
#include <setjmp.h>
//States for child process
#define IDLE 1
#define SERVING 2

//Message exchange types
#define CONNECTION_ESTABLISHED 3 //Sent when new connection is established by child
#define CONNECTION_CLOSED 4 //Sent when connection is closed after request handling
#define RECYCLE_INDICATOR 5

#define MAX_CHILDREN 100
#define MAX_LISTEN 10
#define MAX_CONNECTIONS 2

//union semun {                   /* Used in calls to semctl() */
//    int                 val;
//    struct semid_ds *   buf;
//    unsigned short *    array;
//#if defined(__linux__)
//    struct seminfo *    __buf;
//#endif
//};

//For communication between server parent and child
typedef struct ipc_message{
    pid_t pid;
    int msg_type;
    int connections_held;

}ipc_message;

int get_next_free_id();



