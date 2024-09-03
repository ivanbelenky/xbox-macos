#ifndef READER_H
#define READER_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _PORT 54321
#define MAX_EVENT_LIST_SIZE 16


typedef struct {
    // analogs are 4 bytes, 2 bytes per axis
    // but we approximate only the first byte
    // as the controller is not that sensitive
    // so 1 byte for x, 1 byte for y
    uint8_t left_analog[2];
    uint8_t right_analog[2];
    // for left/right triggers we got two bytes, out of which
    // only 10 bits are used corresponding to the ones 
    // marked with the x'es
    // xxxxxxxx ------xx 
    // 76543210 ------98
    uint8_t left_trigger[2];
    uint8_t right_trigger[2];
    // dpad is encoded in the following way
    // 0000 is no dpad (0)
    // 0001 is up (1)
    // 0010 is up right (2)
    // 0011 is right (3)
    // 0100 is down right (4)
    // 0101 is down (5)
    // 0110 is down left (6)
    // 0111 is left (7)
    // 1000 is up left (8)
    uint8_t dpad;
    // xx-xx-xx
    // 7 bit is right trigger (128)
    // 6 bit is left trigger (64)
    // 4 bit is Y (8)
    // 3 bit is X (4)
    // 1 bit is B (2)
    // 0 bit is A (1)
    uint8_t YXBA;
    // 15th byte
    // -xx-xx--
    // 6 bit Right analog press
    // 5 bit Left analog press
    // 3 bit 3Lines button
    // 2 bit Menu button
    uint8_t extra_buttons;
    // --------x
    // 0 bit is upload button
    uint8_t upload_button;
} xbox_controller_t;


typedef struct {
    bool X;
    bool A;
    bool B;
    bool Y;
    // dpad
    bool UP;
    bool DOWN;
    bool LEFT;
    bool RIGHT;
    // triggers
    bool LB;
    bool RB;
    uint16_t left_trigger;
    uint16_t right_trigger;
    // analogs
    // enough resolutionas it is right now, no need for the extra byte,
    // truly sensitive
    uint8_t left_analog_x;
    uint8_t left_analog_y;
    uint8_t right_analog_x;
    uint8_t right_analog_y;
    // analog press
    bool left_analog_press;
    bool right_analog_press;
    // extra buttons
    bool menu_button;
    bool three_lines_button;
    bool upload_button;
} controller_state_t;


// events are 
// NOOP
// BUTTON_DOWN (button)
// BUTTON_UP (button) 
// TRIGGER_CHANGES_VALUE (trigger, value)
// ANALOG_MOVES (analog, x, y)
// DPAD_MOVES (direction)
typedef enum {
    NOOP = 0,
    BUTTON_DOWN,
    BUTTON_UP,
    TRIGGER_CHANGES_VALUE,
    ANALOG_MOVES,
    DPAD_MOVES
} event_type_t;


typedef enum {
    X = 0,
    Y,
    B,
    A,
    LB,
    RB,
    LEFT_ANALOG_PRESS,
    RIGHT_ANALOG_PRESS,
    MENU_BUTTON,
    THREE_LINES_BUTTON,
    UPLOAD_BUTTON
} button_t;


typedef enum {
    NONE = 0,
    UP,
    UP_RIGHT,
    RIGHT,
    DOWN_RIGHT,
    DOWN,
    DOWN_LEFT,
    LEFT,
    UP_LEFT
} directions_t;


typedef struct {
    event_type_t type;
    union {
        struct {
            button_t button;
        } button_event;
        struct {
            uint16_t value;
            bool left;
        } trigger_event;
        struct {
            uint8_t x;
            uint8_t y;
            bool left;
        } analog_event;
        struct {
            directions_t direction;
        } dpad_event;
    } event;
} event_t;


typedef struct {
    int server_fd;
    int new_socket;
    struct sockaddr_in address;
    socklen_t addrlen;
} socket_server_t;


typedef struct {
    float dt;
    controller_state_t state;
    event_t* event_list;
    socket_server_t* server;
} xbox_context_t; 


int init_socket_server(socket_server_t *server, int PORT);
void close_socket_server(socket_server_t *server);
int send_event(socket_server_t *server, event_t event);


xbox_controller_t parse_xbox_controller(uint8_t *xbox_data);
controller_state_t get_controller_state(xbox_controller_t controller);


#endif