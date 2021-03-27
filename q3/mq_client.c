#include "message_queue.h"


int main(){
    client_init();
    printf("Client has been initialized:\n");
    client_sync client_data;

    send_sync_message(-1, &client_data);
    printf("THe client id is: %d\n", client_data.client_id);








    return 0;
}