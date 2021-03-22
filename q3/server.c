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

void server_init(){
//    sync_key = ftok(CLIENT_SYNC_PASSPHRASE, CLIENT_SYNC_KEY);
//
//    sync_msg_id = msgget(sync_key, 0666 | IPC_CREAT);
//
//    if(sync_msg_id < 0){
//        perror("Error creating sync message queue:");
//        exit(0);
//    }
//    printf("Sync key %d Sync msg queue id: %d\n", sync_key, sync_msg_id);
    server_msg_queue_key = ftok(SERVER_REQ_PASSPHRASE, SERVER_REQ_KEY);
    server_msg_queue_id = msgget(server_msg_queue_key, 0666 | IPC_CREAT);

    if(server_msg_queue_id < 0){
        perror("Error creating server request handler MQ");
        exit(0);
    }
    sync_msg_id = server_msg_queue_id;

    printf("Server mq key %d Server msg queue id: %d\n",server_msg_queue_key,server_msg_queue_id);
    client_msg_reply_id = msgget(CLIENT_SYNC_KEY, 0666 | IPC_CREAT);
    if(client_msg_reply_id < 0){
        perror("Client message queue couldn't be created:\n");
    }
    for(int client_index = 0; client_index < MAX_CLIENTS; client_index ++){
        client_data_ds[client_index].client_id = -1;
        client_data_ds[client_index].num_groups = 0;
        client_data_ds[client_index].is_auto_delete_enabled = false;
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
    printf("MSG queue number: %d\n", sync_msg_id);



    int num_bytes_written = msgsnd(client_msg_reply_id, &syn_packet_reply, sizeof(packet), 0);
    printf("Bytes written: %d\n", num_bytes_written);
    if(num_bytes_written == -1){
        perror("Error sending sync reply:");
        exit(0);
    }
    printf("Server sent response:\n");
    fflush(stdout);
}

bool is_member(int sender_id, int grp_id){
    int member_index =0;
    printf("Sender id: %d\n", sender_id);
    printf("Group id: %d\n", grp_id);
    printf("member of this group: %d", group_table[grp_id][0]);
    while(member_index < MAX_MEMBERS_PER_GRP && group_table[grp_id][member_index] != -1){
        printf("Group id: %d member id %d member %d\n", grp_id, member_index, group_table[grp_id][member_index]);
        if(group_table[grp_id][member_index] == sender_id){
            printf("HEy from in member i return true\n");
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

    while(group_table[grp_id][member_index] != -1){
        int group_member_id = group_table[grp_id][member_index];

        int member_msg_queue_id = msgget(group_member_id, 0);
        printf("Msg queue going to be sent: %d\n", member_msg_queue_id);
        if(member_msg_queue_id < 0){
            perror("Error creating sync message queue:");
            return false;
        }

        if(msgsnd(member_msg_queue_id, &copy_packet, sizeof(packet), 0) == -1){
            perror("Error forwarding message:");
            return false;
        }
        member_index++;
    }
    return true;
}

bool send_private_msg(packet msg_pkt){
    packet copy_packet = msg_pkt;
    message msg = copy_packet.pkt.msg;

    printf("Server time key comp: %ld\n", time(NULL));
//    key_t destination_key = ftok(CLIENT_MQ_PASSPHRASE, copy_msg.destination_id);
//    if(destination_key == -1){
//        perror("Error in key genration:");
//        return false;
//
//    }

//    int destination_msg_queue_id = msgget(destination_key, 0);
    int destination_msg_queue_id = msgget(msg.destination_id, 0);
//    printf("Client pass phrase %s client key %d\n", CLIENT_MQ_PASSPHRASE, destination_key);
    if(destination_msg_queue_id< 0){
        perror("Private msg fail");
        return false;
    }

//    printf("Client id: %d Destination MQ id: %d\n", copy_msg.destination_id, destination_msg_queue_id);



    if(msgsnd(destination_msg_queue_id, &copy_packet, sizeof(packet), 0) == -1){
        perror("Error forwarding message:");
        return false;
    }
    return true;


}
void print_msg_details(message msg){
    printf("Source id %d:\n", msg.source_id);
    printf("Dest id: %d\n", msg.destination_id);
    printf("Message body %s\n", msg.msg_body);
}

/**
 * Handles message communication on the server side, message returned to the client has information about communication
 * status
 */

void message_communication_handler(packet msg_pkt){
     message msg = msg_pkt.pkt.msg;

//    while(true){


    printf("Rcvd message on server side:\n");
    print_msg_details(msg);
    if(msg.is_source_group){
        printf("Going to check is Member:\n");
        if(is_member(msg.source_id, msg.destination_id)){
            printf("In member:\n");

            if(send_group_msg(msg_pkt)){
                strcpy(msg.msg_body, "Successfully sent message");
            }
            else{
                strcpy(msg.msg_body, "Couldn't send message to all members of group");
            }
        }
        else{
            printf("outside member:\n");
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

//        key_t src_key = ftok(CLIENT_MQ_PASSPHRASE, msg.source_id);
//        int src_msg_queue_id = msgget(src_key, 0);
    int src_msg_queue_id = msgget(msg.source_id,0);
    printf("Src ms queue id: %d\n", src_msg_queue_id);
    if(src_msg_queue_id < 0){
        perror("Error creating sync message queue:");
    }
    if(msgsnd(src_msg_queue_id, &msg_pkt, sizeof(packet), 0) == -1){
        perror("Error sending ack:");
    }
    printf("ACL SENT:\n");
//    }

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
    printf("In group handler:\n");

    print_msg_details(msg);

//    if(msgrcv(server_msg_queue_id, &msg, sizeof(message), CREATE_GRP, 0) == -1){
//        perror("Error receiving create group request:");
//        return -1;
//    }
    if(does_group_exist(msg.msg_body)){
        reply_msg.source_id = -1;
        strcpy(reply_msg.msg_body, "Group exists");
    }
    else{
        next_free_grp_id++;
        strcpy(group_name_table[next_free_grp_id], msg.msg_body);
        group_table[next_free_grp_id][0] = msg.source_id;
        printf("Member of group index: %d and client id %d\n", next_free_grp_id, group_table[next_free_grp_id][0]);
        printf("is member %d\n", is_member(msg.source_id, next_free_grp_id));
        strcpy(reply_msg.msg_body, "Group created");
        //storing group id
        reply_msg.source_id = next_free_grp_id;
    }
    int source_queue_id = msgget(msg.source_id, 0);
    reply_pkt.pkt.msg = reply_msg;
    printf("Source queue id: %d\n", source_queue_id);
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


//    if(msgrcv(server_msg_queue_id, &msg, sizeof(message), JOIN_GRP, 0) == -1){
//        perror("Error receiving create group request:");
//        return -1;
//    }
    if(!does_group_exist(msg.msg_body)){
        reply_msg.source_id = -1;
        strcpy(reply_msg.msg_body, "Group doesn't exist");
    }
    else{
        int grp_index = 1;
        for(; grp_index <=next_free_grp_id; next_free_grp_id++){
            if(strcmp(group_name_table[grp_index], msg.msg_body) == 0){
                break;
            }
        }
        //group index is there
        int member_index = 0;
        while(group_table[grp_index][member_index] != -1){
            member_index++;
        }
        group_table[grp_index][member_index] = msg.source_id;
    }
    int source_queue_id = msgget(msg.source_id, 0);
    reply_pkt.pkt.msg = reply_msg;
    if(msgsnd(source_queue_id, &reply_msg, sizeof(message), 0) == -1){
        perror("Error sending join group reply:");
        return -1;
    }
    return 0;

}
int main(){
    server_init();

//    int ret = fork();
//    //Incoming request handler
//    if(ret == 0){
//        while(1){
//            reply_sync_message();
//        }
//    }
//    else{
//        int ret = fork();
//        //Message Communication handler
//        if(ret == 0){
//            while(1){
//                message_communication_handler();
//            }
//        }
//        else{
//            //Join group handler
//            int ret = fork();
//            if(ret == 0){
//                while(1){
//                    create_group_handler();
//                }
//            }
//            else{
//                wait(NULL);
//            }
//
//        }
//    }

    packet msg_pkt;
    while(true){

        int num_bytes_read = msgrcv(server_msg_queue_id, &msg_pkt, sizeof(msg_pkt), 0, 0);
        printf("Hi there server in next round:\n");

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
