#include "prefork_server.h"
int min_idle;
int max_idle;
int MAX_CONNECTIONS;
int curr_idle = 0;
int pid;
int active_connections = 0;

int child_state_info[MAX_CHILDREN][2]; //IDLE or SERVING in [0] and if serving number of connections held in [1]

int pid_map[MAX_CHILDREN]; //stores PID of children
int child_unix_fd[MAX_CHILDREN][2];
int my_fd[2];
fd_set child_comm_set;

//struct sembuf increment;
//struct sembuf decrement;

/**
 * @return Id corresponding to val_to_search (-1: new process request, else existing process id), -1 in case of error
 */
int get_id(int val_to_search){
    for(int index=0;index<MAX_CHILDREN;index++){
        if(pid_map[index] == val_to_search){
            return index;
        }
    }
    return -1;
}

void kill_process(int pid, char *context){
    int index = get_id(pid);
    printf("[%d] Sending SIGTERM to %d: %s\n", getpid(), pid, context);
    kill(pid, SIGTERM);
    child_state_info[index][0] = -1;
    pid_map[index] = -1;
    FD_CLR(child_unix_fd[index][0], &child_comm_set);
    child_unix_fd[index][0] = -1;
    child_unix_fd[index][1] = -1;
    close(child_unix_fd[index][0]);
    close(child_unix_fd[index][1]);

    int status;
    waitpid(pid, &status, 0);

    //printf("Child with pid: %d killed\n",pid);
}

int min(int a, int b){
    if(a < b)
        return a;
    return b;
}


int min_idle_handler(int *spawned_pid_list){
    int num_process_to_spawn = 1;

    int index = 0;
    while(curr_idle < min_idle){
        for(int i=0;i<num_process_to_spawn; i++){
            int id = get_id(-1);
            child_state_info[id][0] = IDLE;
            child_state_info[id][1] = 0;

            socketpair(AF_UNIX, SOCK_STREAM, 0, my_fd);
            pid = fork();

            if(pid == 0){
                close(my_fd[0]);
                break;
            }
            else{

                curr_idle++;
                pid_map[id] = pid;
                //printf("[%d] setting the fd for %d in fdset\n", getpid(), my_fd[0]);
                FD_SET(my_fd[0], &child_comm_set);
                child_unix_fd[id][0] = my_fd[0];
                spawned_pid_list[index++] = pid;
                close(my_fd[1]);

            }
        }
        if(pid == 0){
            break;
        }
        else{
            num_process_to_spawn = min(num_process_to_spawn * 2, 32);

            sleep(1);
        }
    }
    return index;
}

int max_idle_handler(int *spawned_pid_list, int *killed_pid_list){
    int pos = 0;
    while(curr_idle > max_idle){
        int to_kill_pid = spawned_pid_list[pos];
        killed_pid_list[pos] = to_kill_pid;
        kill_process(to_kill_pid, "Max Idle limit process count exceeded");
        curr_idle--;
        pos++;
    }
    return pos;
}

void server_init(int *listen_fd, struct addrinfo *addr_info){
    FD_ZERO(&child_comm_set);
    for(int pos =0; pos < MAX_CHILDREN; pos++){
        child_state_info[pos][0] = -1;
        pid_map[pos] = -1;
        child_unix_fd[pos][0] = -1;
        child_unix_fd[pos][1] = -1;

    }

    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int n =getaddrinfo(NULL, "http", &hints, &res) ;
    if(n  != 0){
        perror("Get addr info call failed:");
        exit(1);
    }
    int ct =0;
    ressave = res;
    while(res !=NULL){
        *listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(*listen_fd < 0){
            continue;
        }
        int op;
        setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(int));
        if(bind(*listen_fd, res->ai_addr, res->ai_addrlen) == 0){

            //printf("Bound at %d'th address family %d  \n", ct, res->ai_family);
            addr_info = res;
            break;
        }
        else{
            perror("Binding error:");
        }
        close(*listen_fd);
        res = res->ai_next;
        ct++;
    }
    if(res == NULL){
        printf("[ERROR] Couldn't bind to any address, try seeing if PORT 80 is used by someother application\n");
        exit(1);
    }
    //printf("The value of listen fd is: %d\n", *listen_fd);
    listen(*listen_fd, MAX_LISTEN);
    freeaddrinfo(ressave);
}

void child_handler(int sig){

    int pid = wait(NULL);
    printf("Sig chld rcvd from: %d", pid);
    active_connections--;
    printf("PID: %d Active connections: %d\n", pid, active_connections);

}

void print_ip_addr(struct addrinfo *res){
    char ipstr[512];
    int ct = 0;
    while(res != NULL){
        ct++;
        struct in_addr *addr;
        if(res->ai_family == AF_INET){
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else{
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
            addr = (struct in_addr *)&(ipv6->sin6_addr);
        }
        inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
//        printf("Family is %d Address is: %s\n", res->ai_family, ipstr);

        res = res->ai_next;
    }
}
//void parent_ack_handler(int sig){
//    printf("Received ack from parent for action\n");
//}

void print_details(int fl){
    printf("\n====================\n");
    if(fl){
        printf("Signal handler update:\n");
    }
    else{
        printf("Process pool change update:\n");
    }
    int num_children =0;
    for(int i=0;i<MAX_CHILDREN; i++){
        if(child_state_info[i][0] != -1){
            num_children++;
        }
    }
    printf("Number of children are %d\n", num_children);

    printf("Process pool:\n");
    for(int i=0;i<MAX_CHILDREN;i++){
        if(child_state_info[i][0] != -1){
            printf(" (PID: %d State: %s Conn held: %d) ", pid_map[i], (child_state_info[i][0] == IDLE ? "IDLE" : "SERVING"), child_state_info[i][1]);
        }
    }
    printf("\n====================\n");
}
int  post_notification_executor(int *spawned_pid_list, int *killed_pid_list){

    min_idle_handler(spawned_pid_list);
    if(pid == 0){
        return 0;
    }
    max_idle_handler(spawned_pid_list, killed_pid_list);
    return 1;
}

void parent_ack_handler(int sig){
    //printf("[%d] Ack recieved from parent:\n", getpid());
}

void sig_int_handler(int sig){
    print_details(1);
}

int main(int argc, char *argv[]){

    //format of running ./a.out --min_spare x --max_spare y --max_req z
    char err_message[200] = "Incorrect usage, supply in format (for ex): ./prefork_server --min_spare 3 --max_spare 4 --max_req 3";
    if(argc != 7){
        printf("%s\n", err_message);
        exit(1);
    }
    if(strcmp(argv[1], "--min_spare") != 0 || strcmp(argv[3], "--max_spare") || strcmp(argv[5], "--max_req") != 0){
        printf("%s\n", err_message);
        exit(1);
    }
    min_idle = atoi(argv[2]);
    max_idle = atoi(argv[4]);
    MAX_CONNECTIONS = atoi(argv[6]);
    if(min_idle > max_idle){
        printf("Cannot have minimum idle connections greater than maximum idle connections\n");
        exit(1);
    }




    int listen_fd;
    struct addrinfo addr_info;
    server_init(&listen_fd, &addr_info);
    //print_ip_addr(&addr_info);
    int *spawned_pid_list = (int *)malloc(sizeof(int) * MAX_CHILDREN);
    int *killed_pid_list = (int *)malloc(sizeof(int) * MAX_CHILDREN);
    for(int index = 0; index < MAX_CHILDREN; index++){
        spawned_pid_list[index] = -1;
    }


    //if(semop(sem_id, &decrement, 1) < 0){
    //    perror("Error operating on semaphore:");
    //}

    int spawned = min_idle_handler(spawned_pid_list);
    //server handler

    if(pid != 0){
        signal(SIGINT, sig_int_handler);

        int killed = max_idle_handler(spawned_pid_list, killed_pid_list);
        //if(semop(sem_id, &increment, 1) < 0){
        //    perror("Error operating on semaphore:");
        //}
        ipc_message buff;

        //Register all the clients
        int maxfd = 0;
        for(int i=0;i<MAX_CHILDREN;i++){
            if(child_unix_fd[i][0] > maxfd){
                maxfd = child_unix_fd[i][0];

            }
        }
        fd_set child_set;
        FD_ZERO(&child_set);
        int intr = 0;
        while(1){
            if(intr == 0){
                print_details(0);
            }
            else{
                intr = 0;
            }
            maxfd = 0;
            for(int i=0;i<MAX_CHILDREN; i++){
                if(child_unix_fd[i][0] > maxfd){
                    maxfd = child_unix_fd[i][0];
                }
            }
            jmp2:
            child_set = child_comm_set;
            int nready = select(maxfd + 1, &child_set, NULL, NULL, NULL);
            if(nready == -1){

                intr = 1;
                if(errno == EINTR){
                    goto jmp2;

                }
                else{
                    perror("Parent select function error: ");
                    exit(1);
                }
            }
            for(int i=0;i<MAX_CHILDREN;i++){
                if(FD_ISSET(child_unix_fd[i][0], &child_set)){
                    ipc_message msg;
                    int nread = read(child_unix_fd[i][0], &msg, sizeof(struct ipc_message));
                    if(nread < 0){
                        perror("Read failed:");
                    }
                    int *spawned_pid_list = (int *)malloc(sizeof(int) * MAX_CHILDREN);
                    int *killed_pid_list = (int *)malloc(sizeof(int) * MAX_CHILDREN);
                    int ret = -1;
                    if(msg.msg_type == RECYCLE_INDICATOR){
                        kill_process(pid_map[i], "Re-cycling connection");
                        ret = post_notification_executor(spawned_pid_list, killed_pid_list);

                    }
                    else if(msg.msg_type == CONNECTION_ESTABLISHED){ //Recvd when first connection is established -- changing state from IDLE to busy
                        int pid = msg.pid;
                        int index = get_id(pid);
                        child_state_info[index][1] = msg.connections_held;
                        if(child_state_info[index][0] == IDLE){
                            child_state_info[index][0] = SERVING;
                            curr_idle--;
                            //printf("Parent sending SIGUSR1 signal to %d\n", pid);
                            kill(pid, SIGUSR1);
                            ret = post_notification_executor(spawned_pid_list, killed_pid_list);
                        }
                        else{
                            kill(pid, SIGUSR1);
                        }
                    }
                    else if(msg.msg_type == CONNECTION_CLOSED){ //
                        int pid = msg.pid;
                        int index = get_id(pid);
                        child_state_info[index][1] = msg.connections_held;
                        if(msg.connections_held == 0){
                            child_state_info[index][0] = IDLE;
                            child_state_info[index][1] = 0;
                            curr_idle++;
                            spawned_pid_list[0] = pid;
                            ret = post_notification_executor(spawned_pid_list, killed_pid_list);
                        }
                        else{
                            kill(pid, SIGUSR1);
                        }

                    }
                    if(ret == 0){
                        //printf("[%d] jumping to child\n", getpid());
                        goto jump;
                    }

                }
            }

        }



    }
    else{
        jump:
        signal(SIGUSR1, parent_ack_handler);
        signal(SIGINT, SIG_IGN);

        char buff[20000];
        strcpy(buff, "HTTP/1.1 200 OK\nDate: Mon, 19 Oct 2009 01:26:17 GMT\nServer: Apache/1.2.6 Red Hat\nContent-Length: 18\nAccept-Ranges: bytes\nKeep-Alive: timeout=15, max=100\nConnection: Keep-Alive\nContent-Type: text/html\n\n<html>Hello</html>");

        int reply_sent = 0;

        fd_set rset, all_set;
        FD_ZERO(&all_set);
        FD_SET(listen_fd, &all_set);
        int maxfd = listen_fd;
        int addr_len = sizeof(struct sockaddr);
        int client_fds[MAX_CONNECTIONS];
        for(int i=0;i<MAX_CONNECTIONS;i++){
            client_fds[i] = -1;
        }
        int active_connections = 0;
        int to_recycle = 0;
        while(1){
            rset = all_set;
            //printf("Max fd limit: %d\n", maxfd + 1);
            int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
            if(nready == -1){
                perror("Error in select: ");
                exit(1);
            }
            //printf("Ready desc are: %d\n", nready);
            if(FD_ISSET(listen_fd, &rset)){
                //printf("[%d] listen descriptors are: %d\n", getpid(), nready);
                int sock_fd = accept(listen_fd, (struct sockaddr *)addr_info.ai_addr, (socklen_t *)&addr_len);
                int i;
                for(i=0;i<MAX_CONNECTIONS;i++){
                    if(client_fds[i] == -1){
                        client_fds[i] = sock_fd;
                        active_connections++;
                        break;
                    }
                }

                if(active_connections == MAX_CONNECTIONS){
                    printf("[%d] Connection limit reached, %d will not accept more connections\n", getpid(), getpid());
                    FD_CLR(listen_fd, &all_set);
                    close(listen_fd);
                    to_recycle = 1;

                }
                //Inform parent you hold 1 more connection
                ipc_message im;
                im.pid = getpid();
                im.msg_type = CONNECTION_ESTABLISHED;
                im.connections_held = active_connections;
                //printf("[%d] Connection establishment notification to parent, active connections: %d\n", getpid(), active_connections);
                write(my_fd[1], &im, sizeof(struct ipc_message));

                pause();
                FD_SET(sock_fd, &all_set);
                if(sock_fd > maxfd){
                    maxfd = sock_fd;
                }

            }
            else{

                for(int i=0;i<MAX_CONNECTIONS;i++){
                    if(client_fds[i] < 0){
                        continue;
                    }
                    char read_buff[4096];
                    if(FD_ISSET(client_fds[i], &rset)){
                       //printf("The ready FD is: %d\n", client_fds[i]);
                       int n = read(client_fds[i], read_buff, 4096);
                       if(n == 0){
                           active_connections--;

                           ipc_message im;
                           im.pid = getpid();
                           im.connections_held =active_connections;
                           if(active_connections == 0 && to_recycle == 1){
                               printf("[%d] Sending recycle indication to parent\n", getpid());
                               im.msg_type = RECYCLE_INDICATOR;
                           }

                           else{
                               im.msg_type = CONNECTION_CLOSED;
                           }


                           //printf("[%d] Connection close notification to parent\n", getpid());

                           close(client_fds[i]);
                           FD_CLR(client_fds[i], &all_set);
                           client_fds[i] = -1;
                           write(my_fd[1], &im, sizeof(ipc_message));
                           pause();
                       }
                       else{
                           //printf("[%d] Writing reply:\n", getpid());
                           sleep(1);
                           reply_sent++;
                           //printf("[%d] socket FD: %d handled request number %d \n", getpid(), client_fds[i], reply_sent);
                           send(client_fds[i], buff, strlen(buff), 0);
                           //printf("Replies sent: %d\n", reply_sent);
                       }

                    }
                }
            }

        }
    }
}