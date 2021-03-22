#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CLIENT_SYNC_PASSPHRASE "client_sync"
#define CLIENT_MQ_PASSPHRASE "client"

#define SERVER_REQ_PASSPHRASE "server_req"

#define CLIENT_SYNC_KEY 614
#define SERVER_REQ_KEY 1228

#define MAX_CLIENTS 100
#define MAX_MEMBERS_PER_GRP 255 //Maximum members who can be in a group
#define MAX_GROUPS 5 // Maximum groups in
#define TIME_OUT 10 //time_out in seconds
#define MAX_PENDING_MESSAGES 50 //Maximum pending messages that can be retrieved
#define MAX_MSG_SIZE 70
#define MAX_GRP_NAME_SZ 10
#define MAX_SYN_REQ 1

#define SEND_MSG 1
#define CREATE_GRP 2
#define JOIN_GRP 3
#define LIST_GRP 4
#define AUTO_DELETE 5
#define EXIT 6
#define SERVER_ACK 7
#define SYN_REQ 8
#define SYN_ACK 9



typedef struct client_sync{
    bool is_auto_delete_enabled; //true if client had set auto delete option
    int client_id; // Client ID that the server provides after registering this client
    int num_groups; // Number of groups that the client is part of
    char group_name[MAX_GROUPS][MAX_GRP_NAME_SZ];
    int group_id[MAX_GROUPS];
}client_sync;

/**
 * @brief client messages can be categorized on the basis of msg_type
 * 1: When client sends sync message, he might have already registered with server or not
 * 2: ACK for this message
 * 3: When he needs to send a message
 */

typedef struct message{
    int source_id; //Client ID / Group ID for sending message, Client ID in case of ACK
    int destination_id;
    bool is_source_group;
    char msg_body[MAX_MSG_SIZE];
    time_t msg_timestamp;
}message;

typedef union packet_component{
    message msg;
    client_sync client_data;

}packet_component;

typedef struct packet{
    long msg_type;
    packet_component pkt;

}packet;

//Server functions
void server_init();
client_sync register_client(int client_id);
void reply_sync_message();
bool is_member(int sender_id, int grp_id);
bool send_group_msg(packet msg_pkt);
bool send_private_msg(packet msg_pkt);
void print_msg_details(message msg);
void message_communication_handler();
bool does_group_exist(char *grp_name);
int create_group_handler();
int join_group_handler();


