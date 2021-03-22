#include "message_queue.h"

client_sync client_sync_data[MAX_CLIENTS]; //stores client info which will eb provided by server at start of client
int present_client_id; // next ID which is assignable for registration






int main() {

    server_init();
//    while(1){
//        send_sync_response();
//    }
    send_sync_response();
    send_sync_response();
    send_sync_response();

}