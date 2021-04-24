#include "rtt.h"

int fi = 0;
int datalen = 56;
int par_pid;
int ct = 0;
int sock_fdv4;
int sock_fdv6;
struct addrinfo *glob;
int pid;


int free_space(){
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    int cleaned = 0;
    for(int index =0; index <BATCHSIZE; index++){
        if(pr[index] != NULL){
            struct timeval t_ref = curr_time;
            tv_sub(&t_ref, &last_requesed_time[index]);
            if(t_ref.tv_sec > SEC_TIMEOUT || (t_ref.tv_sec == SEC_TIMEOUT && t_ref.tv_usec >= MICRO_SEC_TIMEOUT)){//clean this up
                int pings_done = reply_received[index];
                printf("IP address: %s got %d pings -> ", batch_ips[index], pings_done);
                for(int i=0;i<pings_done;i++){
                    printf("RTT%d: %.3f, ", i + 1, rtts[index][i]);
                }
                printf("\n");
                cleaned ++;
                cleanup(index);

            }
        }
    }
    return cleaned;

}
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
    //printf("Sending in V6\n");
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
    *((int *)(icmp6 + 1 + sizeof(struct timeval))) = pr_index;
    *((int *)(icmp6 + 1 + sizeof(struct timeval) + sizeof(int))) = ping_number;
    len = 8 + datalen;
    char *ipv6_addr = malloc(INET6_ADDRSTRLEN);
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)pr[pr_index]->sasend;
    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipv6_addr, INET6_ADDRSTRLEN);
    //printf("IPV6 address is %s\n", ipv6_addr);

    if(sendto (sock_fdv6, sendbuf, len, 0, pr[pr_index]->sasend, pr[pr_index]->salen) == -1){
        //printf("Error num: %d\n", errno);
        perror("Error sending message:");
        exit(1);
    }




#endif
}

void send_v4(int pr_index, int ping_number){
    //printf("Sending in V4\n");
    int len;
    struct icmp *icmp;
    icmp = (struct icmp *)sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = pid;
    icmp->icmp_seq = nsent++;

    //printf("\n==== the packet info is ===\n");



    memset (icmp->icmp_data, 0xa5, datalen);
    gettimeofday ((struct timeval *) icmp->icmp_data, NULL);
    *((int *)(icmp->icmp_data + sizeof(struct timeval))) = pr_index;
    *((int *)(icmp->icmp_data + sizeof(struct timeval) + sizeof(int))) = ping_number;

    len = 8 + datalen;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum ((u_short *) icmp, len);

//    printf("ICMP ID --> %u,  icmp seq number %d icmp cheksum %d\n", icmp->icmp_id, nsent -1, icmp->icmp_cksum);
//
//    printf("===========End of packet info\n\n");
    //printf("Sent %d ping\n", ping_number);
    char *ipv4_addr;
    ipv4_addr = (char *)malloc(INET_ADDRSTRLEN);

    if(sendto (sock_fdv4, sendbuf, len, 0, pr[pr_index]->sasend, pr[pr_index]->salen) == -1){

        perror("Error sending message:");
    }
    else{
        struct sockaddr_in *addr = (struct sockaddr_in *)pr[pr_index]->sasend;
        inet_ntop(AF_INET, &(addr->sin_addr), ipv4_addr, INET_ADDRSTRLEN);
       // printf("Dest IP address is: %s\n", ipv4_addr);
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
    ICMP6_FILTER_SETBLOCKALL(&myfilt);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &myfilt);
    setsockopt(sockfd, IPPROTO_IPV6, ICMP6_FILTER, &myfilt,sizeof (myfilt));
#ifdef IPV6_RECVHOPLIMIT
     setsockopt (sockfd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
#else
     setsockopt (sockfd, IPPROTO_IPV6, IPV6_HOPLIMIT, &on, sizeof(on));
#endif
#endif
}

int store_rtt(double rtt, int index){
    rtts[index][reply_received[index]] = rtt;
    //printf("Rtt stored %.3f \n", rtt);
    reply_received[index]++;
    if(reply_received[index] == PINGS){
        printf("IP address: %s got %d pings -> ", batch_ips[index], PINGS);
        for(int i=1;i<=PINGS;i++){
            printf(" RTT%d %.3f ms, ",i, rtts[index][i-1]);
        }
        printf("\n");

        return 1;
    }
    return 0;
}

int proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv, int index){
    int hlenl, icmplen;
    double rtt;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;

    ip = (struct ip *)ptr;
    hlenl = ip->ip_hl << 2;
    if(ip->ip_p != IPPROTO_ICMP){
        return -1;
    }
    //printf("IP type: %d\n", IPPROTO_ICMP);
    icmp = (struct icmp *)(ptr + hlenl);
    if((icmplen = len - hlenl) < 8){
        return -1;
    }
    if(icmp->icmp_type == ICMP_ECHOREPLY){

        //printf("ICMP id: %d actual pid %d \n", icmp->icmp_id, pid);
        if(icmp->icmp_id != pid)
            return -1;
        if(icmplen < 16)
            return -1;
        //printf("ICMP type is %d\n", ICMP_ECHOREPLY);
        tvsend = (struct timeval *)icmp->icmp_data;
        int aux_data = *(int *)(icmp->icmp_data + sizeof(struct timeval));
        int ping_number = *((int *)(icmp->icmp_data + sizeof(struct timeval) + sizeof(int)));
        //printf("The ping number while processing is: %d\n", ping_number);
        tv_sub(tvrecv, tvsend);


        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
//        printf("Your rtt is: %.3f\n", rtt);
        return store_rtt(rtt, index);

    }

}

int proc_v6(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv, int index){
#ifdef IPV6
    double rtt;
    struct icmp6_hdr *icmp6;
    struct timeval *tvsend;
    struct cmsghdr *cmsg;

    int hlim;
    icmp6 = (struct icmp6_hdr *) ptr;
    if(len < 8)
        return -1;
    if(icmp6->icmp6_type == ICMP6_ECHO_REPLY){
        if(icmp6->icmp6_id !=pid)
            return -1;
        if(len < 16)
            return -1;
        tvsend = (struct timeval *) (icmp6 + 1);
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

        hlim = -1;
        for(cmsg = CMSG_FIRSTHDR(msg); cmsg !=NULL; cmsg = CMSG_NXTHDR(msg, cmsg)){
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
        return store_rtt(rtt, index);
    }
#endif

}

int find_index_from_ip(char *ip){

    for(int i=0;i<BATCHSIZE;i++){
        if(strcmp(ip, batch_ips[i]) == 0){
            return i;
        }
    }
    printf("No index found for this IP\n");
    return -1;

}

void readloop(void){
    //printf("\n\nInto read loop:\n");

    int size;
    char recvbuf[BUFSIZE];
    char controlbuf[BUFSIZE];
    struct msghdr msg;
    struct iovec iov;
    ssize_t n;
    struct timeval tval, select_timeout;
    select_timeout.tv_sec = SEC_TIMEOUT;
    select_timeout.tv_usec = MICRO_SEC_TIMEOUT/2; //wait for 500ms (0.5 s for select to get unblocked)

    fd_set rset, r_cop_set;
    FD_SET(sock_fdv4, &rset);
    FD_SET(sock_fdv6, &rset);

    r_cop_set = rset;
    int max_fd = sock_fdv4;
    if(sock_fdv6 > max_fd)
        max_fd = sock_fdv6;
    int n_ready = select(max_fd + 1, &r_cop_set, NULL, NULL, &select_timeout);
    if(n_ready == 0){
        int cleaned = free_space();
        if(cleaned > 0){
            return;
        }
    }

    setuid(getuid());

    iov.iov_base = recvbuf;
    iov.iov_len = sizeof(recvbuf);

    msg.msg_name = calloc(1, sizeof(struct sockaddr));
    socklen_t len = sizeof(struct sockaddr);

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = controlbuf;
    msg.msg_namelen = len;
    msg.msg_controllen = sizeof(controlbuf);



    int completed = 0;
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
        //Need the index
        struct sockaddr_in *addr = (struct sockaddr_in *)msg.msg_name;
        inet_ntop(AF_INET, &(addr->sin_addr), ipv4_addr, INET_ADDRSTRLEN);

        int index = find_index_from_ip(ipv4_addr);
        if(index == -1){
            printf("invalid ip rcvd %s", ipv4_addr);
            continue;
        }

        gettimeofday(&tval, NULL);


        (pr[index]->fproc)(recvbuf, n, &msg, &tval, index);
        //printf("%d number reply IP for which msg has been received %s\n", reply_received[index], ipv4_addr);
        if(requests_sent[index] < PINGS){
            last_requesed_time[index] = tval;
            pr[index]->fsend(index, reply_received[index] + 1);
            requests_sent[index]++;
        }
        else{
            cleanup(index);
            completed++;
        }
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
        if(index == -1){
            printf("invalid ip rcvd %s", ipv6_addr);
            continue;
        }

        if(init_done[index] == 0){
            (pr[index]->finit)();
            init_done[index] = 1;
        }
        gettimeofday(&tval, NULL);
        (pr[index]->fproc)(recvbuf, n, &msg, &tval, index);
        if(requests_sent[index] < PINGS){
            last_requesed_time[index] = tval;
            pr[index]->fsend(index, reply_received[index] + 1);
            requests_sent[index]++;

        }
        else{
            cleanup(index);
            completed++;
        }
    }
    if(errno != EWOULDBLOCK){
        perror("Error post non block read in IPV6:");
        exit(1);
    }
    if(fcntl(sock_fdv6, F_SETFL, flags) == -1){
        perror("Error restoring flags for IPV6:");
    }

    if(completed > 0){
        return;
    }
    else{
        readloop();
    }

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
    //printf("INTO CLEANING of index %d\n", index);
    free(pr[index]);
    pr[index] = NULL;
    reply_received[index] = 0;
    for(int ping_index=0;ping_index<PINGS;ping_index++){
        rtts[index][ping_index] = 0.0;
    }
    memset(batch_ips[index],0,strlen(batch_ips[index]));
    requests_sent[index] = 0;
    init_done[index] = 0;
    //kill(getpid(), SIGUSR1);
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
    //printf("Socket family %d and socket proto %d\n", AF_INET, IPPROTO_ICMP);
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

//void signal_handler(int sig){
//    printf("SIGUSR1 signal for releasing pause in main loop");
//}
//


int is_pending_ip(){
    for(int i=0;i<BATCHSIZE;i++){
        if(pr[i] != NULL){
            return 1;
        }
    }
    return 0;
}

struct proto* get_proto_structure(int family){
    if(family == AF_INET){
        struct proto *proto_v4 = (struct proto *)malloc(sizeof(struct proto));
        proto_v4->fproc = proc_v4;
        proto_v4->fsend = send_v4;
        proto_v4->finit = NULL;
        proto_v4->icmpproto = IPPROTO_ICMP;
        return proto_v4;
    }
    else if(family == AF_INET6){
        struct proto *proto_v6 = (struct proto *)malloc(sizeof(struct proto));
        proto_v6->fproc = proc_v6;
        proto_v6->fsend = send_v6;
        proto_v6->finit = init_v6;
        proto_v6->icmpproto = IPPROTO_ICMPV6;
        return proto_v6;
    }
    else{
        printf("Wrong family provided\n");
    }
}

int main(int argc, char **argv){
    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);
    pid = getpid() & 0xffff;
    //printf("The pid is %d\n", pid);
    FILE *ip_file = init(argv[1]);
    if(argc != 2){
        printf("Please enter correct number of arguments:\n");
        exit(1);
    }




//    signal(SIGUSR1, signal_handler);
//    signal(SIGALRM, sig_alarm);
    char host[255];
    //alarm(1);
    int ct = 0;
    while(fgets(host, HOSTLEN, ip_file)){
        int host_len = strlen(host);
        host[host_len - 1] = '\0';



        struct addrinfo *ai;
        int index = get_next_free_index();

        if(index == -1) {
            readloop();
            //pause();
            index = get_next_free_index();
        }
        //printf("Index %d host address: %s\n", index,host);
        ai = host_serv(host, NULL, 0,0);
        strcpy(batch_ips[index], host);

        if(ai->ai_family == AF_INET){
            //struct proto proto_v4 = {proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP};
            //printf("Ipv4 address proto %d\n", ai->ai_protocol);
            pr[index] = get_proto_structure(AF_INET);
#ifdef IPV6
            }
        else if(ai->ai_family == AF_INET6){
            //struct proto proto_v6 = {proc_v6, send_v6, NULL, NULL, 0, IPPROTO_ICMPV6};
            //printf("ipv6 address\n");
            pr[index] = get_proto_structure(AF_INET6);
            if(IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr))){
                printf("Cannot ping IPV4 mapped IPV6 address");
            }
#endif
        }
        else{
            printf("Unknown family %d\n", ai->ai_family);
        }
        //TODO: Update this time when you send next request
        gettimeofday(&last_requesed_time[index], NULL);
        pr[index]->sasend = ai->ai_addr;
        pr[index]->sarecv = calloc(1, ai->ai_addrlen);
        pr[index]->salen = ai->ai_addrlen;

        (pr[index]->fsend)(index, 1);

        requests_sent[index]++;
        ct ++;
    }
    while(is_pending_ip()){
        readloop();
    }
    gettimeofday(&t_end, NULL);
    tv_sub(&t_end, &t_start);
    printf("total time %ld (sec) and %ld (micro-sec)\n", t_end.tv_sec, t_end.tv_usec);












}
