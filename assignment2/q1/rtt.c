#include "rtt.h"

struct proto proto_v4 = {proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP};

#ifdef IPV6
struct proto proto_v6 = {proc_v6, send_v6, NULL, NULL, 0, IPPROTO_ICMPV6};
#endif

int datalen = 56;
int ct = 0;
void sig_alrm(int signo){
    (*pr->fsend)();
    ct++;
    if(ct < 3)
        alarm(1);
    return;
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

void send_v6(){
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
    len = 8 + datalen;
    sendto (sockfd, sendbuf, len, 0, pr->sasend, pr->salen);


#endif
}

void send_v4(void){
    int len;
    struct icmp *icmp;
    icmp = (struct icmp *)sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = pid;
    icmp->icmp_seq = nsent++;
    memset (icmp->icmp_data, 0xa5, datalen);
    gettimeofday ((struct timeval *) icmp->icmp_data, NULL);
    printf("%s\n", icmp->icmp_data);
    len = 8 + datalen;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum ((u_short *) icmp, len);
    sendto (sockfd, sendbuf, len, 0, pr->sasend, pr->salen);

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

void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv){
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
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
        printf("%d bytes from seq = %u, ttl=%d, rtt = %.3f ms\n", icmplen, icmp->icmp_seq, ip->ip_ttl, rtt);

    }

}

void proc_v6(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv){
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
        printf("%d bytes from seq=%u, hlim=",len, ,icmp6->icmp6_seq);
        if(hlim == -1){
            printf("???");
        }
        else{
            printf("%d", hlim);
        }
        printf(", rtt=%.3f ms\n", rtt);
    }
#endif

}


void readloop(void){
    int size;
    char recvbuf[BUFSIZE];
    char controlbuf[BUFSIZE];
    struct msghdr msg;
    struct iovec iov;
    ssize_t n;
    struct timeval tval;
    printf("family %d proto %d\n", pr->sasend->sa_family, pr->icmpproto);
    sockfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
    if(sockfd == -1){
        perror("Error creating socket:");
        exit(1);
    }
    printf("The socket fd is: %d\n", sockfd);
    setuid(getuid());

    if(pr->finit)
        (*pr->finit)();
    size = 60 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    sig_alrm(SIGALRM);
    iov.iov_base = recvbuf;
    iov.iov_len = sizeof(recvbuf);

    msg.msg_name = pr->sarecv;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = controlbuf;
    int rcv_count = 0;
    for(;;){
        msg.msg_namelen = pr->salen;
        msg.msg_controllen = sizeof(controlbuf);
        n = recvmsg(sockfd, &msg, 0);
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

int main(int argc, char **argv){
    int c;
    struct addrinfo *ai;
    char *h;

    if(argc != 2){
        printf("Please enter correct number of arguments:\n");
        exit(1);
    }
    host = argv[1];
    pid = getpid() & 0xffff;
    signal(SIGALRM, sig_alrm);

    ai = host_serv(host, NULL, 0,0);

    if(ai->ai_family == AF_INET){
        printf("Ipv4 address proto %d\n", ai->ai_protocol);
        pr = &proto_v4;
        printf("PRROOOOO: %d\n", proto_v4.icmpproto);
#ifdef IPV6
    }
    else if(ai->ai_family == AF_INET6){
        printf("ipv6 address\n");
        pr = &proto_v6;
        if(IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr))){
            printf("Cannot ping IPV4 mapped IPV6 address");
        }
#endif
    }
    else{
        printf("Unknown family %d\n", ai->ai_family);
    }
    pr->sasend = ai->ai_addr;
    pr->sarecv = calloc(1, ai->ai_addrlen);
    pr->salen = ai->ai_addrlen;
    printf("proto here: %d\n", pr->icmpproto);

    readloop();
    exit(0);







}
