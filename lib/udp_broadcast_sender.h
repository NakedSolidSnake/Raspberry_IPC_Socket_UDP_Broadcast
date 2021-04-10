#ifndef UDP_BROADCAST_SENDER_H_
#define UDP_BROADCAST_SENDER_H_

#include <stdbool.h>
#include <stdlib.h>

typedef struct 
{
    int socket;
    const char *hostname;
    const char *port;
} UDP_Sender;

bool UDP_Broadcast_Sender_Init(UDP_Sender *sender);

bool UDP_Broadcast_Sender_Send(UDP_Sender *sender, const char *message, size_t message_size);




#endif /* UDP_BROADCAST_SENDER_H_ */
