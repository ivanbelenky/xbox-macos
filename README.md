# Xbox Controller State to Event mapper

This is a C program that reads input from an Xbox Wireless Controller on macOS and sends the controller events over a network socket. It uses the IOKit framework to interact with the controller and implements a simple socket server to transmit the events. The idea is to map states to events that can be used by other programs to control the system.

## Events
Is a 3 byte event structure

```c
typedef enum {
    NOOP = 0,
    BUTTON_DOWN,
    BUTTON_UP,
    TRIGGER_CHANGES_VALUE,
    ANALOG_MOVES,
    DPAD_MOVES
} event_type_t;
```

### BUTTON EVENTS
```
bytes: 1 | 1 | 1
|--------------|-----------|------|
| \x01 or \x02 | button id | \x00 |
|--------------|-----------|------|
```

### DPAD_EVENTS
```
bytes: 1 | 1 | 1
|------|-----------|------|
| \x05 | direction | \x00 |
|------|-----------|------|
```

### TRIGGER_EVENTS
```
bytes: 1 | 2 
|------|-------------|
| \x03 | value (MSB) |
|------|-------------|
```

### ANALOG_EVENTS
```
bytes: 1 | 1 | 1  
|------|---|---|
| \x04 | x | y |
|------|---|---|
```


## Prerequisites
- macOS
- An Xbox Wireless Controller

## Compilation
To compile the program, use the following command in the terminal:

```bash
gcc socket_server.c -o server -framework IOKit -framework CoreFoundation
```