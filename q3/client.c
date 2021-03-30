#include "message_queue.h"
int sync_msg_id;
key_t sync_key;

key_t client_msg_queue_key;
int client_msg_queue_id;

int server_msg_queue_id;
key_t server_msg_queue_key;

int client_msg_reply_id;

int sem_id;
struct sembuf claim;
struct sembuf release;
#define GROUP "group"
#define PERSON "person"


client_sync send_sync_message(int client_id){
    packet syn_packet, syn_packet_reply;
    syn_packet.msg_type = SYN_REQ;

    client_sync sync_request, sync_reply;

    sync_request.client_id = client_id;
    syn_packet.pkt.client_data = sync_request;

    if(msgsnd(sync_msg_id, &syn_packet, sizeof(packet), 0) == -1){
        perror("Error sending sync message:");
        exit(0);
    }
    printf("Client sends out SYN request:\n");
    if(msgrcv(client_msg_reply_id, &syn_packet_reply, sizeof(packet), SYN_ACK, 0) == -1){
        perror("Error receiving sync message:");
        exit(0);
    }
    printf("Client received SYN ACK:\n");
    return syn_packet_reply.pkt.client_data;
}


void read_pending_messages(client_sync client_ds){
    packet pending_pkt;
    message pending_msg;
    while(1){
        struct tm readable_time_stamp;
        char buf[80];

        if(msgrcv(client_msg_queue_id, &pending_pkt, sizeof(packet), SEND_MSG, 0) == -1){
            perror("Error receiving message from the client Message queue:");
            exit(0);
        }

        pending_msg = pending_pkt.pkt.msg;

        bool is_grp = pending_msg.is_source_group;
        if(semop(sem_id, &claim, 1) < 0){
            perror("Error operating on semaphore:");
        }
        readable_time_stamp = *localtime(&pending_msg.msg_timestamp);
        strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &readable_time_stamp);

        printf("\n===================");
        printf("\nMessage from: %s, client ID: %d, time: %s", (is_grp ? GROUP : PERSON), pending_msg.source_id, buf);
        printf("\nBODY: %s\n", pending_msg.msg_body);
        printf("\n===================\n");

        if(semop(sem_id, &release, 1) < 0){
            perror("Error operating on semaphore:");
        }
    }
}

void list_groups(client_sync client_ds){
    if(client_ds.num_groups == 0){
        printf("You are part of no groups\n");
        return;
    }
    printf("\n===================\n");
    for(int grp_index = 0; grp_index < client_ds.num_groups; grp_index++){
        printf("Group Name: %s group index: %d\n", client_ds.group_name[grp_index], client_ds.group_id[grp_index]);
    }
    printf("\n===================\n");
    fflush(stdout);
}

void print_msg_details(message msg){
    printf("Source id %d:\n", msg.source_id);
    printf("Dest id: %d\n", msg.destination_id);
    printf("Message body %s\n", msg.msg_body);
}
int send_msg(client_sync client_ds){
    packet msg_pkt, reply_msg_pkt;
    message msg, reply_msg;
    msg.timeout = 1e9;
    printf("\n1: send message to group");
    printf("\n2: send message to person");
    int choice;
    printf("\nEnter choice:");
    scanf("%d", &choice);


    if(choice == 1){
        msg.is_source_group = true;
        printf("\nEnter group id:");
    }
    else if(choice == 2){
        msg.is_source_group = false;
        printf("\nEnter client id:");
    }
    else{
        printf("Invalid choice has been entered:\n");
        return -1;
    }
    scanf("%d", &msg.destination_id);
    printf("\nEnter the message:");

    fflush(stdin);
    fflush(stdout);

    fgets(msg.msg_body, MAX_MSG_SIZE, stdin);
    if(choice == 1){
        printf("\nDo you want to enable message timeout(y/n):");
        char c;
        scanf("%c", &c);
        if(c == 'y' || c == 'Y'){
            printf("\nEnter message timeout in sec:");
            scanf("%d", &msg.timeout);
        }
    }
    msg.source_id = client_ds.client_id;
    msg.msg_timestamp = time(NULL);
    msg_pkt.msg_type = SEND_MSG;
    msg_pkt.pkt.msg = msg;

    if(msgsnd(server_msg_queue_id, &msg_pkt, sizeof(packet), 0) == -1){
        perror("Error sending message:\n");
        exit(0);
    }

    if(msgrcv(client_msg_queue_id, &reply_msg_pkt, sizeof(packet), SERVER_ACK, 0) == -1){
        perror("Error receiving ACK:\n");
        exit(0);
    }
    reply_msg = reply_msg_pkt.pkt.msg;
    printf("\n===================\n");
    printf("RESPONSE: %s\n", reply_msg.msg_body);
    printf("\n===================\n");
    return 0;
}

client_sync create_join_group(client_sync client_ds, int req_type){
    printf("Enter group name: ");
    char grp_name[MAX_GRP_NAME_SZ];
    scanf("%s", grp_name);

    message msg, ack_msg;
    packet msg_pkt, ack_msg_pkt;

    msg_pkt.msg_type = CREATE_GRP;
    if(req_type == JOIN_GRP)
        msg_pkt.msg_type = JOIN_GRP;

    msg.source_id = client_ds.client_id;
    msg.msg_timestamp = time(NULL);
    strcpy(msg.msg_body, grp_name);

    msg_pkt.pkt.msg = msg;


    if(msgsnd(server_msg_queue_id, &msg_pkt, sizeof(packet), 0) == -1){
        perror("Error sending group joining request");
        return client_ds;
    }


    if(msgrcv(client_msg_queue_id, &ack_msg_pkt, sizeof(packet), SERVER_ACK, 0) == -1){
        perror("Error receiving create group request ack:");
        return client_ds;
    }
    ack_msg = ack_msg_pkt.pkt.msg;
    printf("\n===================\n");
    printf("RESPONSE: %s\n", ack_msg.msg_body);
    if(ack_msg.source_id != -1){
        int grp_index = client_ds.num_groups;
        if(req_type == CREATE_GRP){
            printf("Created group with id: %d\n", ack_msg.source_id);

        }
        else{
            printf("Joined group with id: %d\n", ack_msg.source_id);
        }
        strcpy(client_ds.group_name[grp_index], grp_name);
        client_ds.group_id[grp_index] = ack_msg.source_id;
        client_ds.num_groups++;
    }
    printf("\n===================\n");
    return client_ds;
}





void client_init(){
    server_msg_queue_key = ftok(SERVER_REQ_PASSPHRASE, SERVER_REQ_KEY);
    server_msg_queue_id = msgget(server_msg_queue_key, 0666 | IPC_CREAT);
    printf("\nServer MQ ID is: %d\n", server_msg_queue_id);

    if(server_msg_queue_id < 0){
        perror("Error creating server request handler MQ");
        exit(errno);
    }
    sync_msg_id = server_msg_queue_id;

    client_msg_reply_id = msgget(CLIENT_SYNC_KEY, 0666 | IPC_CREAT);
    if(client_msg_reply_id < 0){
        perror("Client message queue couldn't be created:");
        exit(errno);
    }

    sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    union semun arg;
    arg.val = 1;
    if(sem_id < 0){
        perror("Error creating semaphore:");
        exit(1);
    }
    if (semctl(sem_id, 0, SETVAL, arg) == -1)
    {
        perror("Error initializing semaphore:");
        exit(1);
    }
    claim.sem_num = 0;
    claim.sem_flg = 0;
    claim.sem_op = -1;

    release.sem_num = 0;
    release.sem_flg = 0;
    release.sem_op = 1;
}

void print_menu(){
    printf("\n1: Send Message:\n");
    printf("2: Create Group:\n");
    printf("3: Join Group:\n");
    printf("4: List Groups:\n");
    printf("5: Exit\n");
}

int main(){
    client_init();
    bool is_registered = false;
    int client_id;
    client_sync client_data;
    while(!is_registered){
        printf("Enter your client ID (or) enter the -1 if you are coming first time:");
        scanf("%d", &client_id);
        client_data = send_sync_message(client_id);
        if(client_data.client_id == -1){
            printf("Invalid client ID has been entered:\n");
        }
        else{
            printf("You have registered with ID: %d\n", client_data.client_id);
            int client_id = client_data.client_id;
            client_msg_queue_id = msgget(client_id, 0666 | IPC_CREAT);
            if(client_msg_queue_id < 0){
                perror("Error creating client MQ");
                exit(0);
            }
            is_registered = true;
        }
    }

    //Client has registered and has the structure in client_data
    int ret = fork();
    if(ret == 0){
        while(1){
            read_pending_messages(client_data);
        }
    }
    else{
        int user_option;
        while(1){
            if(semop(sem_id, &claim, 1) < 0){
                perror("Error operating on semaphore:");
            }
            print_menu();

            printf("\nEnter choice:");
            if(semop(sem_id, &release, 1) < 0){
                perror("Error operating on semaphore:");
            }
            scanf("%d", &user_option);

            switch(user_option){
                case SEND_MSG:
                    send_msg(client_data);
                    break;
                case JOIN_GRP:
                    client_data = create_join_group(client_data, JOIN_GRP);
                    break;
                case CREATE_GRP:
                    client_data = create_join_group(client_data, CREATE_GRP);
                    break;
                case LIST_GRP:
                    list_groups(client_data);
                    break;
                case EXIT:
                    break;

                default:
                    break;

            }
            if(user_option == EXIT){
                break;
            }

        }
    }
}
