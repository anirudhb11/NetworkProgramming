#include "message_queue.h"
int sync_msg_id;
key_t sync_key;

int server_msg_queue_id;
key_t server_msg_queue_key;

int client_msg_reply_id;


client_sync client_data_ds[MAX_CLIENTS];
int next_free_id; // Identifier available for person
int next_free_grp_id; // Identifier available for group

int group_table[MAX_GROUPS][MAX_MEMBERS_PER_GRP]; //stores client IDs part of this group
char group_name_table[MAX_GROUPS][MAX_GRP_NAME_SZ];
packet queued_group_messages[MAX_GROUPS][MAX_QUEUED_MESSAGES]; //queues group messages
int num_queued_messages[MAX_GROUPS]; //Number of queued messages per group

void server_init(){
    server_msg_queue_key = ftok(SERVER_REQ_PASSPHRASE, SERVER_REQ_KEY);
    server_msg_queue_id = msgget(server_msg_queue_key, 0666 | IPC_CREAT);

    if(server_msg_queue_id < 0){
        perror("Error creating server request handler MQ");
        exit(0);
    }
    sync_msg_id = server_msg_queue_id;

    printf("Server msg queue id: %d\n",server_msg_queue_id);
    client_msg_reply_id = msgget(CLIENT_SYNC_KEY, 0666 | IPC_CREAT);
    if(client_msg_reply_id < 0){
        perror("Client message queue couldn't be created:\n");
    }
    for(int client_index = 0; client_index < MAX_CLIENTS; client_index ++){
        client_data_ds[client_index].client_id = -1;
        client_data_ds[client_index].num_groups = 0;
    }
    for(int grp_index =0; grp_index < MAX_GROUPS; grp_index++){
        for(int client_index =0; client_index < MAX_MEMBERS_PER_GRP; client_index++){
            group_table[grp_index][client_index] = -1;
        }
    }
}

client_sync register_client(int client_id){
    if(client_id == -1){
        next_free_id ++;
        client_data_ds[next_free_id].client_id = next_free_id;
        return client_data_ds[next_free_id];
    }
    else{
        return client_data_ds[client_id];
    }

}

void reply_sync_message(packet syn_packet){
    packet syn_packet_reply;
    client_sync cli_data;
    client_sync sync_request = syn_packet.pkt.client_data;

    cli_data = register_client(sync_request.client_id);
    syn_packet_reply.pkt.client_data = cli_data;
    syn_packet_reply.msg_type = SYN_ACK;

    int num_bytes_written = msgsnd(client_msg_reply_id, &syn_packet_reply, sizeof(packet), 0);
    if(num_bytes_written < 0){
        perror("Error sending sync reply:");
        exit(0);
    }
    printf("Server sent SYN response for client id %d:\n", cli_data.client_id);
    fflush(stdout);
}

bool is_member(int sender_id, int grp_id){
    int member_index =0;
    while(member_index < MAX_MEMBERS_PER_GRP && group_table[grp_id][member_index] != -1){
        if(group_table[grp_id][member_index] == sender_id){
            return true;
        }
        member_index++;
    }
    return false;
}
/**
 *
 * @param msg that needs to be forwarded to the group
 * @return true if successfully sent, else false the msg body has error message which is sent to the client
 */

bool send_group_msg(packet msg_pkt){
    packet copy_packet = msg_pkt;
    message msg = copy_packet.pkt.msg;
    int grp_id = msg.destination_id;
    int member_index = 0;

    //Queued message
    int queue_msg_index = num_queued_messages[grp_id];
    queued_group_messages[grp_id][queue_msg_index] = copy_packet;
    num_queued_messages[grp_id]++;

    while(group_table[grp_id][member_index] != -1){
        int group_member_id = group_table[grp_id][member_index];

        int member_msg_queue_id = msgget(group_member_id, 0);
        if(member_msg_queue_id < 0){
            perror("Error creating sync message queue:");
            return false;
        }

        if(msgsnd(member_msg_queue_id, &copy_packet, sizeof(packet), 0) == -1){
            perror("Error forwarding message:");
            return false;
        }
        printf("Server wrote to msg queue %d: client id: %d\n", member_msg_queue_id, group_member_id);
        member_index++;
    }
    return true;
}

bool send_private_msg(packet msg_pkt){
    packet copy_packet = msg_pkt;
    message msg = copy_packet.pkt.msg;

    int destination_msg_queue_id = msgget(msg.destination_id, 0);
    if(destination_msg_queue_id< 0){
        perror("Private msg fail");
        return false;
    }
    if(msgsnd(destination_msg_queue_id, &copy_packet, sizeof(packet), 0) == -1){
        perror("Error forwarding message:");
        return false;
    }
    printf("Server wrote to msg queue %d client id %d\n", destination_msg_queue_id, msg_pkt.pkt.msg.destination_id);
    return true;


}
void print_msg_details(message msg){
    printf("\n============\n");
    printf("Source id %d:\n", msg.source_id);
    printf("is destination group: %d Dest id: %d\n", msg.is_source_group, msg.destination_id);
    printf("Message body %s\n", msg.msg_body);
    printf("\n============\n");
}

/**
 * Handles message communication on the server side, message returned to the client has information about communication
 * status
 */

void message_communication_handler(packet msg_pkt){

    message msg = msg_pkt.pkt.msg;
    printf("Message communication handler invoked\n");
    print_msg_details(msg);

    printf("Received message on server side:\n");
    print_msg_details(msg);
    if(msg.is_source_group){
        if(is_member(msg.source_id, msg.destination_id)){
            if(send_group_msg(msg_pkt)){
                strcpy(msg.msg_body, "Successfully sent message");
            }
            else{
                strcpy(msg.msg_body, "Couldn't send message to all members of group");
            }
        }
        else{
            strcpy(msg.msg_body, "You are not a member of this group");
        }

    }
    else{
        if(send_private_msg(msg_pkt)){
            strcpy(msg.msg_body, "Successfully sent message");
        }
        else{
            strcpy(msg.msg_body, "Message couldn't be delivered");
        }

    }
    //send ack
    msg_pkt.msg_type = SERVER_ACK;
    msg_pkt.pkt.msg = msg;

    int src_msg_queue_id = msgget(msg.source_id,0);
    if(src_msg_queue_id < 0){
        perror("Error creating sync message queue:");
    }
    if(msgsnd(src_msg_queue_id, &msg_pkt, sizeof(packet), 0) == -1){
        perror("Error sending ack:");
    }
    printf("ACK SENT:\n");
}

bool does_group_exist(char *grp_name){
    for(int grp_index = 1; grp_index <= next_free_grp_id; grp_index++){
        if(strcmp(group_name_table[grp_index], grp_name) == 0){
            return true;
        }
    }
    return false;

}

int create_group_handler(packet msg_pkt){
    message msg = msg_pkt.pkt.msg;
    packet reply_pkt;
    reply_pkt.msg_type = SERVER_ACK;
    message reply_msg;
    printf("Create group handler invoked\n");
    print_msg_details(msg);

    if(does_group_exist(msg.msg_body)){
        reply_msg.source_id = -1;
        strcpy(reply_msg.msg_body, "Group exists");
    }
    else{
        next_free_grp_id++;
        strcpy(group_name_table[next_free_grp_id], msg.msg_body);
        group_table[next_free_grp_id][0] = msg.source_id;
        strcpy(reply_msg.msg_body, "Group created");
        //storing group id
        reply_msg.source_id = next_free_grp_id;
        client_sync* client_data = &client_data_ds[msg.source_id];

        strcpy(client_data->group_name[client_data->num_groups], msg.msg_body);
        client_data->group_id[client_data->num_groups] = next_free_grp_id;
        client_data->num_groups++;
    }
    int source_queue_id = msgget(msg.source_id, 0);
    reply_pkt.pkt.msg = reply_msg;
    if(msgsnd(source_queue_id, &reply_pkt, sizeof(packet), 0) == -1){
        perror("Error sending create group reply:");
        return -1;
    }
    return 0;
}

int join_group_handler(packet msg_pkt){
    packet reply_pkt;
    message msg = msg_pkt.pkt.msg;
    message reply_msg;
    reply_pkt.msg_type = SERVER_ACK;
    int grp_index = 1;
    printf("Join group handler invoked\n");
    print_msg_details(msg);

    if(!does_group_exist(msg.msg_body)){
        reply_msg.source_id = -1;
        strcpy(reply_msg.msg_body, "Group doesn't exist");
    }
    else{
        for(; grp_index <=next_free_grp_id; grp_index++){
            if(strcmp(group_name_table[grp_index], msg.msg_body) == 0){
                break;
            }
        }
        reply_msg.source_id = grp_index;
        if(is_member(msg.source_id, grp_index)){
            strcpy(reply_msg.msg_body, "You are already a member of this group");
            reply_msg.source_id = -1;
        }
        //group index is there

        else{
            client_sync* client_data = &client_data_ds[msg.source_id];

            int member_index = 0;
            while(group_table[grp_index][member_index] != -1){
                member_index++;
            }
            group_table[grp_index][member_index] = msg.source_id;
            strcpy(reply_msg.msg_body, "Join group request succeeded");

            strcpy(client_data->group_name[client_data->num_groups], group_name_table[grp_index]);
            client_data->group_id[client_data->num_groups] = grp_index;
            client_data->num_groups++;
            printf("Client data num groups %d\n", client_data->num_groups);
        }

    }
    int source_queue_id = msgget(msg.source_id, 0);
    reply_pkt.pkt.msg = reply_msg;
    if(msgsnd(source_queue_id, &reply_pkt, sizeof(packet), 0) == -1){
        perror("Error sending join group reply:");
        return -1;
    }
    printf("Join request ACK sent to queue id %d client id %d\n", source_queue_id, msg.source_id);

    //Pending messages handler
    for(int queue_message_index = 0; queue_message_index < num_queued_messages[grp_index]; queue_message_index++){
        packet msg_packet = queued_group_messages[grp_index][queue_message_index];

        message msg = (struct message)msg_packet.pkt.msg;
        time_t curr_time = time(NULL);
        if(curr_time - msg.msg_timestamp <= msg.timeout){
            if(msgsnd(source_queue_id, &msg_packet, sizeof(packet), 0) == -1){
                perror("Error sending group queued messages:");
            }
        }
        else{
            printf("\nThe message exceeded the timeout hence won't be delivered");
        }

    }
    return 0;

}
int main(){
    server_init();

    packet msg_pkt;
    while(true){
        int num_bytes_read = msgrcv(server_msg_queue_id, &msg_pkt, sizeof(msg_pkt), 0, 0);
        if (num_bytes_read == -1) {
            perror("Error reading off the server queue");
            exit(0);
        }
        if(msg_pkt.msg_type == SYN_REQ){
            reply_sync_message(msg_pkt);
        }
        if (msg_pkt.msg_type == SEND_MSG) {
            message_communication_handler(msg_pkt);
        }
        if (msg_pkt.msg_type == CREATE_GRP) {
            create_group_handler(msg_pkt);
        }
        if (msg_pkt.msg_type == JOIN_GRP) {
            join_group_handler(msg_pkt);
        }
    }
}
