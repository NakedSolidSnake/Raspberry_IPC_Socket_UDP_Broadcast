#include <udp_broadcast_sender.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

bool UDP_Broadcast_Sender_Init(UDP_Sender *sender)
{
    int broadcast_enable;    
    bool status = false;

    do 
    {
        if(!sender)
            break;

        sender->socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(sender->socket < 0)
            break;

        if(setsockopt(sender->socket, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast_enable, sizeof(broadcast_enable)) < 0)
            break;

        status = true;        
    }while(false);
    
    return status;
}

bool UDP_Broadcast_Sender_Send(UDP_Sender *sender, const char *message, size_t message_size)
{
    bool status = false;
    struct sockaddr_in server;
    ssize_t send_len;

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(sender->hostname);
    server.sin_port = htons(atoi(sender->port));

    send_len = sendto(sender->socket, message, message_size, 0, (struct sockaddr *)&server, sizeof(server));
    if(send_len == message_size)
        status = true;

    return status;
}