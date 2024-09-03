#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "reader.h"


int init_socket_server(socket_server_t *server, int PORT) {
    if ((server->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return -1;
    }
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(PORT);
    server->addrlen = sizeof(server->address);


    if (bind(server->server_fd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0) {
        perror("bind failed");
        close(server->server_fd);
        return -1;
    }

    if (listen(server->server_fd, 3) < 0) {
        perror("listen failed");
        close(server->server_fd);
        return -1;
    }
    
    printf("Waiting for connection...\n\n");
    if ((server->new_socket = accept(server->server_fd, (struct sockaddr *)&server->address, (socklen_t*)&server->addrlen)) < 0) {
        printf("accept failed\n\n");
        close(server->server_fd);
        return -1;
    }
    return 0;
}

void close_socket_server(socket_server_t *server) {
    close(server->new_socket);
    close(server->server_fd);
}

int send_event(socket_server_t *server, event_t event) {
    uint8_t bytes[3]; // Maximum size needed for any event type

    switch (event.type) {
        case BUTTON_UP:
        case BUTTON_DOWN:
            bytes[0] = (uint8_t)event.type;
            bytes[1] = (uint8_t)event.event.button_event.button;
            break;
        case DPAD_MOVES:
            bytes[0] = (uint8_t)event.type;
            bytes[1] = (uint8_t)event.event.dpad_event.direction;
            break;        
        case TRIGGER_CHANGES_VALUE:
            bytes[0] = (uint8_t)event.type | (event.event.trigger_event.left << 7);
            bytes[1] = (uint8_t)((event.event.trigger_event.value >> 8) & 0xFF);
            bytes[2] = (uint8_t)(event.event.trigger_event.value & 0xFF);
            break;
        case ANALOG_MOVES:
            bytes[0] = (uint8_t)event.type | (event.event.trigger_event.left << 7);
            bytes[1] = (uint8_t)(event.event.analog_event.x);
            bytes[2] = (uint8_t)(event.event.analog_event.y);
            break;
        default:
            return -1;
    }
    return send(server->new_socket, bytes, 3, 0);
}