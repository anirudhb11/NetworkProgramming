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
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/in_systm.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#define BUFSIZE 4096
#define BATCHSIZE 32 // Max number of pending requests at any point of time
#define PINGS 3 //Number of pings per ip
#define THRESHOLD 1 //is the maximum time we wait for a ping
#define HOSTLEN 128
char sendbuf[BUFSIZE];


int datalen;
char *host;
int nsent;
pid_t pid;
int sockfd;

void init_v6(void);
void proc_v4(char *, ssize_t, struct msghdr *, struct timeval *, int);
void proc_v6(char *, ssize_t, struct msghdr *, struct timeval *, int);
void send_v4(int, int);
void send_v6(int, int);
void readloop(void);
void sig_alrm(int);
void tv_sub(struct timeval *, struct timeval *);
void cleanup(int index);

struct proto{
    void (*fproc)(char *, ssize_t, struct msghdr *, struct timeval *, int);
    void (*fsend)(int, int);
    void (*finit)(void);
    struct sockaddr *sasend;
    struct sockaddr *sarecv;
    socklen_t salen;
    int icmpproto;
} *pr[BATCHSIZE];

int reply_received[BATCHSIZE];
struct timeval last_requesed_time[BATCHSIZE];
double rtts[BATCHSIZE][PINGS];
char *batch_ips[BATCHSIZE];
int init_done[BATCHSIZE];


#ifdef IPV6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif
