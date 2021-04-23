
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef PITREX
// Pitrex includes

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "window.h"

static int screen_width = 0;
static int screen_height = 0;

#elif PITREX_PLAYGROUND
// Pitrex calls forwarded to playground

int currentButtonState = 0;
int currentJoy1X = 0;
int currentJoy1Y = 0;
static int usePipeline = 1;
static int screen_width = 0;
static int screen_height = 0;

extern void platform_set_size(int width, int height);

int vectrexinit(char viaconfig);
void v_setName(char *name);
void v_init(void);
void v_setRefresh(int hz);
void v_setBrightness(int color) {}
void v_WaitRecal(void);
void v_readButtons(void);
void v_readJoystick1Analog(void);

void v_window(int left, int bottom, int right, int top, boolean clipping) { platform_set_size(right-left, top-bottom); screen_width = right-left; screen_height = top-bottom; }
void v_directDraw32(int x1, int y1, int x2, int y2, int color);
void v_printString(int x, int y, char* str, int textSize, int color);
int  v_printStringRaster(int8_t x, int8_t y, char* str, int8_t xSize, int8_t ySize, unsigned char delimiter);

#else
#error Either PITREX or PITREX_PLAYGROUND must defined
#endif

int run_web_server(int port);
void game_handle_remote_request(char* request);

int remoteButtonState = 0;
int remoteCurrentJoy1X = 0;
int remoteCurrentJoy1Y = 0;

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))

// MARK: - Platform

void platform_init(char* name, int width, int height, int hz) {
    screen_width = width;
    screen_height = height;
    
    vectrexinit(1);
#ifndef FREESTANDING
    v_setName(name);
#endif
    v_init();
    usePipeline = 1;
#ifdef FREESTANDING
    v_setupIRQHandling();
    v_enableJoystickAnalog(1,1,0,0);
    v_enableButtons(1);
#endif

    v_setRefresh(hz);
    v_setBrightness(DEFAULT_COLOR);
    v_window(0, 0, width, height, false);
}

void platform_frame(void) {
    v_WaitRecal();
    v_readButtons();
    v_readJoystick1Analog();
}

// MARK: - Input handling

static int get_current_joy1_x() {
    if (remoteCurrentJoy1X != 0)
        return remoteCurrentJoy1X;
    
    return currentJoy1X;
}

static int get_current_joy1_y() {
    if (remoteCurrentJoy1Y != 0)
        return remoteCurrentJoy1Y;

    return currentJoy1Y;
}

static int get_button_state() {
    if (remoteButtonState != 0)
        return remoteButtonState;

    return currentButtonState;
}

boolean platform_input_is_left(void) {
    if (get_current_joy1_x() < -50) {
        printf("Left\n");
        return true;
    }
    
    return false;
}

boolean platform_input_is_right(void) {
    if (get_current_joy1_x() > 50) {
        printf("Right\n");
        return true;
    }
    
    return false;
}

boolean platform_input_is_up(void) {
    if (get_current_joy1_y() > 50) {
        printf("Up\n");
        return true;
    }
    
    return false;
}

boolean platform_input_is_down(void) {
    if (get_current_joy1_y() < -50) {
        printf("Down\n");
        return true;
    }
    
    return false;
}

boolean platform_button_is_pressed(int number) {
    int buttonState = get_button_state();
    
    switch (number) {
        case 1:
            if ((buttonState&0x01) == (0x01))
                return true;
            break;
        case 2:
            if ((buttonState&0x02) == (0x02))
                return true;
            break;
        case 3:
            if ((buttonState&0x04) == (0x04))
                return true;
            break;
        case 4:
            if ((buttonState&0x08) == (0x08))
                return true;
            break;

        default:
            return false;
    }
    
    return false;
}

// MARK: - Platform Input control

void platform_set_control_state(int code, boolean press) {
    switch (code) {
        case 1:   // Joystick left
            remoteCurrentJoy1X = press ? -127 : 0;
            printf("Set Joy1X: %d\n", remoteCurrentJoy1X);
            break;
        case 2:   // Joystick right
            remoteCurrentJoy1X = press ? 127 : 0;
            printf("Set Joy1X: %d\n", remoteCurrentJoy1X);
            break;
        case 3:   // Joystick down
            remoteCurrentJoy1Y = press ? -127 : 0;
            printf("Set Joy1Y: %d\n", remoteCurrentJoy1Y);
            break;
        case 4:   // Joystick up
            remoteCurrentJoy1Y = press ? 127 : 0;
            printf("Set Joy2X: %d\n", remoteCurrentJoy1Y);
            break;
        case 5:   // Button 1
            if (press)
                BIT_SET(remoteButtonState, 0);
            else
                BIT_CLEAR(remoteButtonState, 0);
            printf("Set Button 5 state: %d\n", remoteButtonState);
            break;
        case 6:   // Button 2
            if (press)
                BIT_SET(remoteButtonState, 1);
            else
                BIT_CLEAR(remoteButtonState, 1);
            printf("Set Button 6 state: %d\n", remoteButtonState);
            break;
        case 7:   // Button 3
            if (press)
                BIT_SET(remoteButtonState, 2);
            else
                BIT_CLEAR(remoteButtonState, 2);
            printf("Set Button 7 state: %d\n", remoteButtonState);
            break;
        case 8:   // Button 4
            if (press)
                BIT_SET(remoteButtonState, 3);
            else
                BIT_CLEAR(remoteButtonState, 3);
            printf("Set Button 8 state: %d\n", remoteButtonState);
            break;
    }
}

boolean platform_get_control_state(int code) {
    switch (code) {
        // Buttons Joystick 1
        case 1:
            return platform_button_is_pressed(1);
        case 2:
            return platform_button_is_pressed(2);
        case 3:
            return platform_button_is_pressed(3);
        case 4:
            return platform_button_is_pressed(4);

       // Directions Joystick 1
        case 5:
            return platform_input_is_left();
        case 6:
            return platform_input_is_right();
        case 7:
            return platform_input_is_up();
        case 8:
            return platform_input_is_down();

        default:
            return false;
    }
}

// MARK: - Remote input handling

#ifndef FREESTANDING

#include <pthread.h>

static void* threadFunction(void* args) {
    printf("Start remote input server\n");
    run_web_server(33333);

    return 0;
}
#endif

void platform_start_remote_input() {
#ifndef FREESTANDING
    pthread_t id;
    int ret;
    
    // creating thread
    ret = pthread_create(&id, NULL, &threadFunction,NULL);
    if (ret != 0) {
        printf("Remote server failed to start.\n");
    }
#endif
}

int process_command(char* command, int parameter) {
#ifndef FREESTANDING
    if (strcmp(command, "press") == 0 && parameter != -1) {
        platform_set_control_state(parameter, true);
    }
    else if (strcmp(command, "release") == 0 && parameter != -1) {
        platform_set_control_state(parameter, false);
    }
    else {
        return 0;
    }
#endif

    return 1;
}

int process_request(char* request) {
#ifdef FREESTANDING
    return 1;
#else
    char* pos = strchr(request, '?');
    
    char command[256];
    if (pos == NULL) {
        strncpy(command, request+1, strlen(request)-1);
        return process_command(command, -1);
    }
    else {
        char* token = strtok(request, "?");
        char* command = token+1;
        char* temp = strtok(NULL, " ");
        int parameter = -1;
        
        if (temp != NULL) {
            parameter = atoi(temp);
        }
        
        if (!process_command(command, parameter)) {
            game_handle_remote_request(request);
        }
    }

    return 1;
#endif
}

// MARK: - Text drawing

void platform_msg(char* msg, int x, int y, int size, int color) {
    v_printString(x, y, msg, size, color);
}

void platform_raster_msg(char* msg, int x, int y, int size, int color) {
#ifdef PITREX
    v_setBrightness(color);
    v_printStringRaster(x, y, msg, size, -7, 0);
#else
    platform_msg(msg, x, y, size, color);
#endif
}

// MARK: - Line drawing

void platform_draw_line(int x1, int y1, int x2, int y2, int color) {
#ifdef PITREX
    // int xx1 = MAX_DAC * x1 / 400 - DAC;
    // int yy1 = MAX_DAC * y1 / 400 - DAC;
    // int xx2 = MAX_DAC * x2 / 400 - DAC;
    // int yy2 = MAX_DAC * y2 / 400 - DAC;

    // v_directDraw32(xx1, yy1, xx2, yy2, color);
    v_line(x1, y1, x2, y2, color);
#else
    v_directDraw32(x1, y1, x2, y2, color);
#endif
}

void platform_draw_lines(int* points, int count, int color) {
    int index = 2;
    int* offset = points;

    //printf("Draw points: %d (%d)\n", count, color);
    
    if (count < 2)
        return;

    if (count % 2 != 0)
        return;

    int x1 = *offset;
    offset++;
    int y1 = *offset;
    offset++;

    if (count == 2) {
        platform_draw_line(x1, y1, x1, y1, color);
        return;
    }

    while (index < count) {
        int x2 = *offset;
        x2 += x1;
        offset++;
        
        int y2 = *offset;
        y2 += y1;
        offset++;

        // printf(" > %d,%d,%d,%d\n", x1, y1, x2, y2);

        platform_draw_line(x1, y1, x2, y2, color);
        
        x1 = x2;
        y1 = y2;
        
        index += 2;
    }
}

#ifdef PITREX
double platform_get_ms(void) {
    return v_millis() / 1000.0f;
}
#endif

#ifdef PITREX
static char full_path[256];
const char* platform_bundle_file_path(const char* filename, const char* extension) {
    sprintf(full_path, "%s.%s", filename, extension);
    return full_path;
}
#endif
