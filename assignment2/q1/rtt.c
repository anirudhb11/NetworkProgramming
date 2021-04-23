#include "rtt.h"


int datalen = 56;
int par_pid;
int ct = 0;
int sock_fdv4;
int sock_fdv6;
struct addrinfo *glob;

uint16_t in_cksum(uint16_t *addr, int len){
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t  answer = 0;

    while(nleft > 1){
        sum += *w++;
        nleft -= 2;
    }
    if(nleft == 1){
        * (unsigned char *) (&answer) = * (unsigned char *) w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

void send_v6(int pr_index, int ping_number){
#ifdef IPV6
    int len;
    struct icmp6_hdr *icmp6;

    icmp6 = (struct icmp6_hdr *) sendbuf;
    icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
    icmp6->icmp6_code = 0;
    icmp6->icmp6_id = pid;
    icmp6->icmp6_seq = nsent++;
    memset ((icmp6 + 1), 0xa5, datalen);
    gettimeofday ((struct timeval *) (icmp6 + 1), NULL);
    *((int *)(icmp6->icmp_data + sizeof(struct timeval))) = pr_index;
    *((int *)(icmp6->icmp_data + sizeof(struct timeval) + sizeof(int))) = ping_number;
    len = 8 + datalen;

    if(sendto (sock_fdv6, sendbuf, len, 0, pr[pr_index]->sasend, pr[pr_index]->salen) == -1){

        perror("Error sending message:");
    }




#endif
}

void send_v4(int pr_index, int ping_number){
    int len;
    struct icmp *icmp;
    icmp = (struct icmp *)sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = pid;
    icmp->icmp_seq = nsent++;
    memset (icmp->icmp_data, 0xa5, datalen);
    gettimeofday ((struct timeval *) icmp->icmp_data, NULL);
    *((int *)(icmp->icmp_data + sizeof(struct timeval))) = pr_index;
    *((int *)(icmp->icmp_data + sizeof(struct timeval) + sizeof(int))) = ping_number;

    len = 8 + datalen;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum ((u_short *) icmp, len);
    printf("Sent %d ping\n", ping_number);
    char *ipv4_addr;
    ipv4_addr = (char *)malloc(INET_ADDRSTRLEN);
    if(sendto (sock_fdv4, sendbuf, len, 0, pr[pr_index]->sasend, pr[pr_index]->salen) == -1){

        perror("Error sending message:");
    }
    else{
        struct sockaddr_in *addr = (struct sockaddr_in *)pr[pr_index]->sasend;
        inet_ntop(AF_INET, &(addr->sin_addr), ipv4_addr, INET_ADDRSTRLEN);
        printf("Dest IP address is: %s\n", ipv4_addr);
    }


}

void tv_sub(struct timeval *out, struct timeval *in){
    if((out->tv_usec -= in->tv_usec) < 0){
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void init_v6(){
#ifdef IPV6
    int on = 1;
    struct icmp6_filter myfilt;
    ICMP6_FILTER_SEtBLOCKALL(&myfilt);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &myfilt);
    setsockopt(sockfd, IPPROTO_IPV6, ICMP6_FILTER, &myfilt,sizeof (myfilt));
#ifdef IPV6_RECVHOPLIMIT
     setsockopt (sockfd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
#else
     setsockopt (sockfd, IPPROTO_IPV6, IPV6_HOPLIMIT, &on, sizeof(on));
#endif
#endif
}

void store_rtt(double rtt, int index){
    rtts[index][reply_received[index]] = rtt;
    reply_received[index]++;
    if(reply_received[index] == PINGS){
        printf("IP address: %s -> ", batch_ips[index]);
        for(int i=1;i<=PINGS;i++){
            printf(" RTT%d %.3f, ",i, rtts[index][i-1]);
        }
        printf("\n");
        cleanup(index);

    }
}

void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv, int index){
    int hlenl, icmplen;
    double rtt;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;

    ip = (struct ip *)ptr;
    hlenl = ip->ip_hl << 2;
    if(ip->ip_p != IPPROTO_ICMP){
        return;
    }
    icmp = (struct icmp *)(ptr + hlenl);
    if((icmplen = len - hlenl) < 8){
        return;
    }
    if(icmp->icmp_type == ICMP_ECHOREPLY){
        if(icmp->icmp_id != pid)
            return;
        if(icmplen < 16)
            return;

        tvsend = (struct timeval *)icmp->icmp_data;
        int aux_data = *(int *)(icmp->icmp_data + sizeof(struct timeval));
        tv_sub(tvrecv, tvsend);


        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
        store_rtt(rtt, index);



    }

}

void proc_v6(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv, int index){
#ifdef IPV6
    double rtt;
    struct icmp6_hdr *icmp6;
    struct timeval *tvsend;
    struct cmsghdr *cmg;

    int hlim;
    icmp6 = (struct icmp6_hdr *) ptr;
    if(len < 8)
        return;
    if(icmp6->icmp6_type == ICMP6_ECHO_REPLY){
        if(icmp6->icmp6_id !=pid)
            return;
        if(len < 16)
            return;
        tvsend = (struct timeval *) (icmp6 + 1);
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

        hlim = -1;
        for(cmsg = CMG_FIRSTHDR(msg); cmsg !=NULL; cmsg = CMSG_NXTHDR(msg, cmsg)){
            if(cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_HOPLIMIT){
                hlim = * (u_int32_t *) CMSG_DATA(cmsg);
                break;
            }
        }
        /*printf("%d bytes from seq=%u, hlim=",len, ,icmp6->icmp6_seq);
        if(hlim == -1){
            printf("???");
        }
        else{
            printf("%d", hlim);
        }
        printf(", rtt=%.3f ms\n", rtt);
        */
        store_rtt(rtt, index);
    }
#endif

}

int find_index_from_ip(char *ip){
    return 0;
    for(int i=0;i<BATCHSIZE;i++){
        if(strcmp(ip, batch_ips[i]) == 0){
            return i;
        }
    }
    printf("No index found for this IP\n");
    exit(1);

}

void readloop(void){
    printf("Into read loop:\n");
    int size;
    char recvbuf[BUFSIZE];
    char controlbuf[BUFSIZE];
    struct msghdr msg;
    struct iovec iov;
    ssize_t n;
    struct timeval tval;
    socklen_t len = glob->ai_addrlen;
    printf("glob %d from sock address %ld\n", len, sizeof(struct sockaddr));

    setuid(getuid());


    int n_bytes;
    int is_init_done = 0;



    //TODO: Needs to be moved into the for loop for message specific stuff
    //if(pr->finit)
    //    (*pr->finit)();

    //sig_alrm(SIGALRM);
    iov.iov_base = recvbuf;
    iov.iov_len = sizeof(recvbuf);

//    msg.msg_name = calloc(1, sizeof(struct sockaddr));
    msg.msg_name = pr[0]->sarecv;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = controlbuf;

    //make this socket non blocking
    int flags = fcntl(sock_fdv4, F_GETFL, 0);
    if(flags == -1){
        perror("Error obtaining IPV4 socket flags:");
    }
    if(fcntl(sock_fdv4, F_SETFL, flags | O_NONBLOCK) == -1){
        perror("Error setting non blocking flag for IPV4 socket");
    }

    //Read IPV4 messages
    char *ipv4_addr;
    ipv4_addr = (char *)malloc(INET_ADDRSTRLEN);

    while((n = recvmsg(sock_fdv4, &msg, 0)) != -1){
        msg.msg_namelen = pr[0]->salen;
        msg.msg_controllen = sizeof(controlbuf);
        //Need the index
        struct sockaddr_in *addr = (struct sockaddr_in *)msg.msg_name;
        inet_ntop(AF_INET, &(addr->sin_addr), ipv4_addr, INET_ADDRSTRLEN);
        printf("IP rcvd packet is: %s numbytes is %ld\n", ipv4_addr, n);
        int index = find_index_from_ip(ipv4_addr);

        gettimeofday(&tval, NULL);
        (pr[index]->fproc)(recvbuf, n, &msg, &tval, index);


    }
    if(errno != EWOULDBLOCK){
        perror("Error post non block read in IPV4:");
        exit(1);
    }
    if(fcntl(sock_fdv4, F_SETFL, flags) == -1){
        perror("Error restoring flags for IPV4:");
    }

    flags = fcntl(sock_fdv6, F_GETFL, 0);
    if(flags == -1){
        perror("Error obtaining IPV6 socket flags:");
    }
    if(fcntl(sock_fdv6, F_SETFL, flags | O_NONBLOCK) == -1){
        perror("Error setting non blocking flag for IPV6 socket:");
    }
    char *ipv6_addr;
    ipv6_addr = (char *)malloc(INET6_ADDRSTRLEN);
    while((n = recvmsg(sock_fdv6, &msg, 0 )) != -1){
        msg.msg_namelen = len;
        msg.msg_controllen = sizeof(controlbuf);
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)msg.msg_name;
        inet_ntop(AF_INET6, &(addr->sin6_addr), ipv6_addr, INET6_ADDRSTRLEN);
        int index = find_index_from_ip(ipv6_addr);
        if(init_done[index] == 0){
            (pr[index]->finit)();
        }
        gettimeofday(&tval, NULL);
        (pr[index]->fproc)(recvbuf, n, &msg, &tval, index);
    }
    if(errno != EWOULDBLOCK){
        perror("Error post non block read in IPV6:");
        exit(1);
    }
    if(fcntl(sock_fdv6, F_SETFL, flags) == -1){
        perror("Error restoring flags for IPV6:");
    }
    /*for(;;){
        msg.msg_namelen = pr->salen;
        msg.msg_controllen = sizeof(controlbuf);
        n = recvmsg(sockfd, &msg, 0);
        printf("Family is: %u   \n", ((struct sockaddr *)(msg.msg_name))->sa_family);
        if(n < 0){
            if(errno == EINTR)
                continue;
            else{
                perror("Error in rcv message");
                exit(1);
            }
        }
        gettimeofday(&tval, NULL);
        (*pr->fproc)(recvbuf, n, &msg, &tval);
        rcv_count++;
        if(rcv_count == 3)
            return;
    }*/
}

struct addrinfo *host_serv(const char *hostname, const char *service, int family, int socktype){
    int n;
    struct addrinfo hints, *res;
    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_CANONNAME | AI_NUMERICHOST;
    hints.ai_family = family;
    hints.ai_socktype = socktype;

    if((n = getaddrinfo(hostname, service, &hints, &res)) != 0){
        return NULL;
        printf("Get addr info err\n");
    }
    return res;
}

void cleanup(int index){
    printf("INTO CLEANING\n");
    pr[index] = NULL;
    reply_received[index] = 0;
    for(int ping_index=0;ping_index<PINGS;ping_index++){
        rtts[index][ping_index] = 0.0;
    }
    init_done[index] = 0;
    kill(par_pid, SIGUSR1);
}

FILE* init(char *fname){
    for(int batch_index=0; batch_index < BATCHSIZE; batch_index++){
        for(int ping_index =0; ping_index < PINGS; ping_index++){
            rtts[batch_index][ping_index] = 0.0;
        }
        init_done[batch_index] = 0;
        pr[batch_index] = NULL;
        batch_ips[batch_index] = (char *)malloc(sizeof(char) * HOSTLEN);
        reply_received[batch_index] = 0;
    }
    FILE *ip_file;
    if((ip_file = fopen(fname, "r")) == NULL){
        printf("Could not open file\n");
        exit(1);
    }
    printf("Socket family %d and socket proto %d\n", AF_INET, IPPROTO_ICMP);
    sock_fdv4 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sock_fdv4 == -1){
        perror("Error creating IPV4 socket:");
        exit(1);
    }
    sock_fdv6 = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if(sock_fdv6 == -1){
        perror("Error creating IPV6 socket:");
        exit(1);
    }
    int size = 60 * 1024;
    if(setsockopt(sock_fdv6, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == -1){
        perror("Error setting socket options on IPV6 socket");
        exit(1);
    }
    if(setsockopt(sock_fdv4, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == -1){
        perror("Error setting socket options on IPV4 socket");
        exit(1);
    }
    return ip_file;

}

//-1 means full state all IPs are still pending responses
int get_next_free_index(){
    for(int i=0;i<BATCHSIZE; i++){
        if(pr[i] == NULL){
            return i;
        }
    }
    return -1;
}

void signal_handler(int sig){
    printf("SIGUSR1 signal for releasing pause in main loop");
}

void sig_alarm(int sig){

    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);

    for(int index =0; index <BATCHSIZE; index++){
        if(pr[index] != NULL){
            struct timeval t_ref = curr_time;
            tv_sub(&t_ref, &last_requesed_time[index]);
            if(t_ref.tv_sec >= 7){//clean this up
                printf("diff %ld\n", t_ref.tv_sec);
                cleanup(index);
            }
        }
    }
    alarm(1);
}

int main(int argc, char **argv){
    par_pid = getpid();
    FILE *ip_file = init(argv[1]);
    if(argc != 2){
        printf("Please enter correct number of arguments:\n");
        exit(1);
    }
    int n;
    char host[HOSTLEN];
    signal(SIGUSR1, signal_handler);
    signal(SIGALRM, sig_alarm);
    alarm(1);
    while((n = fscanf(ip_file, "%[^\n]",host)) != 0){

        printf("host address %s\n", host);
        struct addrinfo *ai;
        int index = get_next_free_index();
        printf("index is %d\n", index);
        if(index == -1) {

            index = get_next_free_index();
        }
        pid = getpid() & 0xffff;
        ai = host_serv(host, NULL, 0,0);
        glob = ai;
        strcpy(batch_ips[index], host);

        if(ai->ai_family == AF_INET){
            struct proto proto_v4 = {proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP};
            printf("Ipv4 address proto %d\n", ai->ai_protocol);
            pr[index] = &proto_v4;
#ifdef IPV6
            }
        else if(ai->ai_family == AF_INET6){
            struct proto proto_v6 = {proc_v6, send_v6, NULL, NULL, 0, IPPROTO_ICMPV6};
            printf("ipv6 address\n");
            pr[index] = &proto_v6;
            if(IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr))){
                printf("Cannot ping IPV4 mapped IPV6 address");
            }
#endif
        }
        else{
            printf("Unknown family %d\n", ai->ai_family);
        }
        gettimeofday(&last_requesed_time[index], NULL);
        pr[index]->sasend = ai->ai_addr;
        pr[index]->sarecv = calloc(1, ai->ai_addrlen);
        pr[index]->salen = ai->ai_addrlen;

        for(int ping = 0; ping < PINGS; ping++){
            (pr[index]->fsend)(index, ping + 1);
        }

    }
    sleep(2);
    readloop();









}
