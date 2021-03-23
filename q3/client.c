#include "message_queue.h"
int sync_msg_id;
key_t sync_key;


key_t client_msg_queue_key;
int client_msg_queue_id;


int server_msg_queue_id;
key_t server_msg_queue_key;

int client_msg_reply_id;

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
    printf("Client syn reply id: %d\n", client_msg_reply_id);
    if(msgrcv(client_msg_reply_id, &syn_packet_reply, sizeof(packet), SYN_ACK, 0) == -1){
        perror("Error receiving sync message:");
        exit(0);
    }
    printf("Client rcvd data:\n");
    return syn_packet_reply.pkt.client_data;
}

/**
 * @return true if the message can be delivered to the client on the basis of timeout
 */
bool is_deliverable(time_t msg_write_time){
    time_t curr_time = time(NULL);
    if(curr_time - msg_write_time < TIME_OUT){
        return true;
    }
    else{
        return false;
    }
}

void read_pending_messages(client_sync client_ds){

    packet pending_pkt;
    message pending_msg;
    while(1){
        if(msgrcv(client_msg_queue_id, &pending_pkt, sizeof(packet), SEND_MSG, 0) == -1){
            perror("Error receiving message from the client Message queue:");
            exit(0);
        }
        pending_msg = pending_pkt.pkt.msg;
        if(!client_ds.is_auto_delete_enabled || is_deliverable(pending_msg.msg_timestamp)){
            bool is_grp = pending_msg.is_source_group;
            printf("\n Message from: %s, client ID: %d\n", (is_grp ? GROUP : PERSON), pending_msg.source_id);
            printf("\n %s\n", pending_msg.msg_body);
        }
    }
}

void list_groups(client_sync client_ds){
    if(client_ds.num_groups == 0){
        printf("You are part of no groups\n");
        return;
    }
    for(int grp_index = 0; grp_index < client_ds.num_groups; grp_index++){
        printf("Group Name: %s group index: %d\n", client_ds.group_name[grp_index], client_ds.group_id[grp_index]);
    }
}

void print_msg_details(message msg){
    printf("Source id %d:\n", msg.source_id);
    printf("Dest id: %d\n", msg.destination_id);
    printf("Message body %s\n", msg.msg_body);
}
int send_msg(client_sync client_ds){
    packet msg_pkt, reply_msg_pkt;
    message msg, reply_msg;
    printf("1: send message to group\n");
    printf("2: send message to person\n");
    int choice;
    printf("Enter choice:");
    scanf("%d", &choice);
    printf("The choice you chose: %d\n", choice);

    if(choice == 1){
        msg.is_source_group = true;
        printf("Enter group id:\n");
    }
    else if(choice == 2){
        msg.is_source_group = false;
        printf("Enter client id:\n");
    }
    else{
        printf("Invalid choice has been entered:\n");
        return -1;
    }
    scanf("%d", &msg.destination_id);
    printf("\nEnter the message:\n");

    scanf("%s", msg.msg_body);

    msg.source_id = client_ds.client_id;
    msg.msg_timestamp = time(NULL);
    msg_pkt.msg_type = SEND_MSG;
    msg_pkt.pkt.msg = msg;

    print_msg_details(msg);
    if(msgsnd(server_msg_queue_id, &msg_pkt, sizeof(packet), 0) == -1){
        perror("Error sending message:\n");
        exit(0);
    }



    if(msgrcv(client_msg_queue_id, &reply_msg_pkt, sizeof(packet), SERVER_ACK, 0) == -1){
        perror("Error receiving ACK:\n");
        exit(0);
    }
    reply_msg = reply_msg_pkt.pkt.msg;
    printf("%s\n", reply_msg.msg_body);

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
    printf("Going to receive from %d client id\n", client_msg_queue_id);

    if(msgrcv(client_msg_queue_id, &ack_msg_pkt, sizeof(packet), SERVER_ACK, 0) == -1){
        perror("Error receiving create group request ack:");
        return client_ds;
    }
    ack_msg = ack_msg_pkt.pkt.msg;
    printf("%s\n", ack_msg.msg_body);
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
    return client_ds;
}





void client_init(){
//    sync_key = ftok(CLIENT_SYNC_PASSPHRASE, CLIENT_SYNC_KEY);
//    sync_msg_id = msgget(sync_key, 0666 | IPC_CREAT);
//    if(sync_msg_id < 0){
//        perror("Error creating sync message queue:");
//        exit(0);
//    }

    server_msg_queue_key = ftok(SERVER_REQ_PASSPHRASE, SERVER_REQ_KEY);
    server_msg_queue_id = msgget(server_msg_queue_key, 0666 | IPC_CREAT);

    if(server_msg_queue_id < 0){
        perror("Error creating server request handler MQ");
        exit(0);
    }
    sync_msg_id = server_msg_queue_id;

    client_msg_reply_id = msgget(CLIENT_SYNC_KEY, 0666);
    if(client_msg_reply_id < 0){
        perror("Client message queue couldn't be created:\n");
    }
}

void print_menu(){
    printf("1: Send Message:\n");
    printf("2: Create Group:\n");
    printf("3: Join Group:\n");
    printf("4: List Groups:\n");
    printf("5: Enable auto-delete:\n");
    printf("6: Exit\n");
}

int main(){
    printf("Size of message is %lu\n", sizeof(message));
    printf("Sizeof client ds: %lu\n", sizeof(client_sync));
    printf("The size of the packet is: %lu\n", sizeof(struct packet));
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
            printf("Client side MQ creation: %ld\n", time(NULL));
//            client_msg_queue_key = ftok(CLIENT_MQ_PASSPHRASE, client_id);
//            client_msg_queue_id = msgget(client_msg_queue_id, 0666 | IPC_CREAT);
            client_msg_queue_id = msgget(client_id, 0666 | IPC_CREAT);
            printf("Client's MQ is: %d\n", client_msg_queue_id);
            if(client_msg_queue_id < 0){
                perror("Error creating client MQ");
                exit(0);
            }
//            printf("My passphrase is %s My key is %d My mq is: %d\n", CLIENT_MQ_PASSPHRASE, client_msg_queue_key, client_msg_queue_id);
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
            print_menu();
            printf("Enter choice:\n");
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

                default:
                    break;

            }

        }
        kill(ret, SIGKILL);

    }







}
