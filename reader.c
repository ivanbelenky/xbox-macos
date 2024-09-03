#include <stdio.h>
#include <stdlib.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>

#include "reader.h"


xbox_controller_t get_controller_data(uint8_t *buffer, uint32_t length) {
    xbox_controller_t controller = {0};

    controller.left_analog[0] = buffer[2];
    controller.left_analog[1] = buffer[4];
    controller.right_analog[0] = buffer[6];
    controller.right_analog[1] = buffer[8];
    controller.left_trigger[0] = buffer[9];
    controller.left_trigger[1] = buffer[10];
    controller.right_trigger[0] = buffer[11];
    controller.right_trigger[1] = buffer[12];
    controller.dpad = buffer[13];
    controller.YXBA = buffer[14];
    controller.extra_buttons = buffer[15];
    controller.upload_button = buffer[16];

    return controller;
}

controller_state_t get_controller_state(xbox_controller_t controller) {
    controller_state_t state = {0};
    
    state.Y = (bool)(controller.YXBA & 0xF0);
    state.X = (bool)(controller.YXBA & 0x08);
    state.B = (bool)(controller.YXBA & 0x02);
    state.A = (bool)(controller.YXBA & 0x01);
    
    state.UP = (bool)((controller.dpad == 1) || (controller.dpad == 2) || (controller.dpad == 8));
    state.DOWN = (bool)((controller.dpad == 5) || (controller.dpad == 4) || (controller.dpad == 6));
    state.LEFT = (bool)((controller.dpad == 7) || (controller.dpad == 6) || (controller.dpad == 8));
    state.RIGHT = (bool)((controller.dpad == 3) || (controller.dpad == 2) || (controller.dpad == 4));

    state.RB = (bool)(controller.YXBA & 0x80);
    state.LB = (bool)(controller.YXBA & 0x40);
    
    state.left_trigger = ((controller.left_trigger[1]) << 8) + controller.left_trigger[0];
    state.right_trigger = ((controller.right_trigger[1]) << 8) + controller.right_trigger[0];

    state.left_analog_x = controller.left_analog[0];
    state.left_analog_y = controller.left_analog[1];

    state.right_analog_x = controller.right_analog[0];
    state.right_analog_y = controller.right_analog[1];

    state.right_analog_press = (bool)(controller.extra_buttons & 0x40);
    state.left_analog_press = (bool)(controller.extra_buttons & 0x20);

    state.three_lines_button = (bool)(controller.extra_buttons & 0x08);
    state.menu_button = (bool)(controller.extra_buttons & 0x04);
    state.upload_button = (bool)(controller.upload_button & 0x01);

    return state;
}

void get_buttons(controller_state_t state, bool* buffer) {
    static bool buttons[11]; // Ensure the array is static to avoid returning a pointer to a local variable.
    
    buffer[0] = state.X;
    buffer[1] = state.Y;
    buffer[2] = state.B;
    buffer[3] = state.A;
    buffer[4] = state.LB;
    buffer[5] = state.RB;
    buffer[6] = state.left_analog_press;
    buffer[7] = state.right_analog_press;
    buffer[8] = state.menu_button;
    buffer[9] = state.three_lines_button;
    buffer[10] = state.upload_button;
}

event_t* get_events(controller_state_t *prev_state, controller_state_t *current_state, event_t* event_list) {
    event_t* events = event_list;
    for (int i = 0; i < 16; i++) {events[i] = (event_t){0};}
    int event_counter = 0;
    bool prev_buttons_buf[11];
    bool current_buttons_buf[11];
    get_buttons(*prev_state, prev_buttons_buf);
    get_buttons(*current_state, current_buttons_buf);
    
    for (int i = 0; i < 11; i++) {
        if (prev_buttons_buf[i] != current_buttons_buf[i]) {
            event_t event = {0};
            event.type = current_buttons_buf[i] ? BUTTON_DOWN : BUTTON_UP;
            event.event.button_event.button = i;
            events[event_counter] = event;
            event_counter++;
        }
    }

    if (prev_state->left_trigger != current_state->left_trigger) {
        event_t event = {0};
        event.type = TRIGGER_CHANGES_VALUE;
        event.event.trigger_event.value = current_state->left_trigger;
        event.event.trigger_event.left = true;
        events[event_counter] = event;
        event_counter++;
    }

    if (prev_state->right_trigger != current_state->right_trigger) {
        event_t event = {0};
        event.type = TRIGGER_CHANGES_VALUE;
        event.event.trigger_event.value = current_state->right_trigger;
        event.event.trigger_event.left = false;
        events[event_counter] = event;
        event_counter++;
    }

    if (prev_state->left_analog_x != current_state->left_analog_x || prev_state->left_analog_y != current_state->left_analog_y) {
        event_t event = {0};
        event.type = ANALOG_MOVES;
        event.event.analog_event.x = current_state->left_analog_x;
        event.event.analog_event.y = current_state->left_analog_y;
        event.event.analog_event.left = true;
        events[event_counter] = event;
        event_counter++;
    }

    if (prev_state->right_analog_x != current_state->right_analog_x || prev_state->right_analog_y != current_state->right_analog_y) {
        event_t event = {0};
        event.type = ANALOG_MOVES;
        event.event.analog_event.x = current_state->right_analog_x;
        event.event.analog_event.y = current_state->right_analog_y;
        event.event.analog_event.left = false;
        events[event_counter] = event;
        event_counter++;
    }

    // xor the dpad states to detect changes
    if (
        prev_state->UP != current_state->UP || prev_state->DOWN != current_state->DOWN || 
        prev_state->LEFT != current_state->LEFT || prev_state->RIGHT != current_state->RIGHT
    ) {
        event_t event = {0};
        event.type = DPAD_MOVES;
        event.event.dpad_event.direction = (current_state->UP << 0) | (current_state->RIGHT << 1) | (current_state->DOWN << 2) | (current_state->LEFT << 3);
        events[event_counter] = event;
        event_counter++;
    }
    return events;
}


void handle_input_report(void *context, IOReturn result, void *sender, IOHIDReportType type, uint32_t reportID, uint8_t *report, CFIndex reportLength) {
    xbox_context_t *ctx = (xbox_context_t*)context;
    xbox_controller_t controller_data = get_controller_data(report, reportLength);
    controller_state_t new_state = get_controller_state(controller_data);
    controller_state_t prev_state = ctx->state;    
    event_t* event_list = ctx->event_list;
    
    event_list = get_events(&prev_state, &new_state, event_list);
    ctx->state = new_state;
    
    for (int i = 0; i < 16; i++) {
        event_t event = event_list[i];
        if (event.type != 0) {
            if (send_event(ctx->server, event) < 0) {
                perror("send failed");
                break;
            };
        };
    };
}


IOHIDDeviceRef find_xbox_controller(IOHIDManagerRef manager) {
    IOHIDDeviceRef device = NULL;

    CFSetRef deviceSet = IOHIDManagerCopyDevices(manager);
    if (deviceSet == NULL) {
        fprintf(stderr, "Failed to copy HID devices\n");
        return NULL;
    }

    CFIndex count = CFSetGetCount(deviceSet);
    fprintf(stdout, "Number of HID devices found: %ld\n", count);
    if (count == 0) {
        fprintf(stderr, "No HID devices found\n");
        CFRelease(deviceSet);
        return NULL;
    }

    IOHIDDeviceRef *deviceRefs = (IOHIDDeviceRef *)malloc(sizeof(IOHIDDeviceRef) * count);
    if (deviceRefs == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        CFRelease(deviceSet);
        return NULL;
    }

    CFSetGetValues(deviceSet, (const void **)deviceRefs);
    for (CFIndex i = 0; i < count; i++) {
        CFStringRef product = (CFStringRef)IOHIDDeviceGetProperty(deviceRefs[i], CFSTR(kIOHIDProductKey));
        if (product != NULL) {
            char productName[256];
            if (CFStringGetCString(product, productName, sizeof(productName), kCFStringEncodingUTF8)) {
                if (strstr(productName, "Xbox Wireless Controller") != NULL) {
                    device = deviceRefs[i];
                    break;
                }
            }
        }
    }

    free(deviceRefs);
    CFRelease(deviceSet);

    return device;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        return EXIT_FAILURE;
    }
    int PORT;
    if (argc == 2) {
        PORT = atoi(argv[1]);
    } else {
        PORT = _PORT;
    };

    IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (CFGetTypeID(hidManager) != IOHIDManagerGetTypeID()) {
        fprintf(stderr, "Failed to create HID manager\n");
        return EXIT_FAILURE;
    }

    IOHIDManagerSetDeviceMatching(hidManager, NULL);
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    IOReturn result = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    if (result != kIOReturnSuccess) {
        fprintf(stderr, "Failed to open HID manager\n");
        CFRelease(hidManager);
        return EXIT_FAILURE;
    }

    IOHIDDeviceRef xboxController = find_xbox_controller(hidManager);
    if (xboxController == NULL) {
        fprintf(stderr, "Xbox controller not found\n");
        CFRelease(hidManager);
        return EXIT_FAILURE;
    }

    uint8_t buffer[17];
    xbox_context_t context = {0};
    event_t* events_list = (event_t *)malloc(sizeof(event_t) * MAX_EVENT_LIST_SIZE);
    context.dt = time(NULL);
    context.event_list = events_list;
    context.state = (controller_state_t){0};

    socket_server_t* server = (socket_server_t*)malloc(sizeof(socket_server_t));
    context.server = server;

    printf("Xbox controller found\n");

    if (init_socket_server(server, PORT) != 0) {
        fprintf(stderr, "Failed to initialize socket server\n");
        CFRelease(hidManager);
        return EXIT_FAILURE;
    }

    IOHIDDeviceRegisterInputReportCallback(xboxController, buffer, 17, handle_input_report, &context);
    IOHIDDeviceScheduleWithRunLoop(xboxController, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    printf("Listening for input reports from Xbox controller...\n");
    CFRunLoopRun();

    CFRelease(hidManager);
    free(events_list);
    free(server);
    return EXIT_SUCCESS;
}
