#include "message_queue.h"


/**
 * @brief: Prints availbale options for the client.
 */
void print_menu(){
    printf("1. Register with Server\n");
    printf("2. Send private message\n");
    printf("3. Create group\n");
    printf("4. Join group\n");
    printf("5. Send group message\n");
    printf("6. List groups\n");
    printf("7. Receive messages\n");
    printf("8. Enable auto-delete option\n");
}



int main(){
    client_init();
    printf("Client has been initialized:\n");
    client_sync client_data;

    send_sync_message(-1, &client_data);
    printf("THe client id is: %d\n", client_data.client_id);








    return 0;
}