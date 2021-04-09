
#include "platform.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef PITREX
// Pitrex includes

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#elif PITREX_PLAYGROUND
// Pitrex calls forwarded to playground

int currentButtonState;
int currentJoy1X;
int currentJoy1Y;

int vectrexinit(char viaconfig);
void v_setName(char *name);
void v_init(void);
void v_setRefresh(int hz);

void v_WaitRecal(void);
void v_readButtons(void);
void v_readJoystick1Analog(void);

void v_directMove32(int32_t x1, int32_t y1);
void v_directDeltaDraw32(int32_t x1, int32_t y1, uint8_t color);

void v_directDraw32(int x1, int y1, int x2, int y2, int color);
void v_printString(int x, int y, char* str, int textSize, int color);
int  v_printStringRaster(int8_t x, int8_t y, char* str, int8_t xSize, int8_t ySize, unsigned char delimiter);

#else
// Pitrex calls outputed to stdio

#include <unistd.h>

int currentButtonState;
int currentJoy1X;
int currentJoy1Y;

int vectrexinit(char viaconfig) { printf("vectrexinit\n"); return 0; }
void v_setName(char *name) { printf("v_setName: %s\n", name); }
void v_init(void) { printf("v_init\n"); }
void v_setRefresh(int hz) { printf("v_setRefresh: %d\n", hz); }

void v_WaitRecal() { printf("v_WaitRecal ------------------\n"); usleep(20*1000); }
void v_readButtons() { printf("v_readButtons\n"); }
void v_readJoystick1Analog() { printf("v_readJoystick1Analog\n"); }

void v_directMove32(int32_t x1, int32_t y1)  { printf("v_directMove32: %d,%d\n", x1, y1); }
void v_directDeltaDraw32(int32_t x1, int32_t y1, uint8_t color) { printf("v_directDeltaDraw32: %d,%d (%d)\n", x1, y1, color); }

void v_directDraw32(int x1, int y1, int x2, int y2, int color) { printf("v_directDraw32: %d,%d,%d,%d (%d)\n", x1, y1, x2, y2, color); }
void v_printString(int x, int y, char* str, int textSize, int color) { printf("v_printString: %s\n", str); }
int  v_printStringRaster(int8_t x, int8_t y, char* str, int8_t xSize, int8_t ySize, unsigned char delimiter) { printf("v_printStringRaster: %s\n", str); return 0; }

#endif

#define MAX_DAC 32000
#define DAC MAX_DAC/2

boolean platform_wait = false;
int platform_wait_count = 0;

// MARK: - Platform

void platform_init(char* name) {
    vectrexinit(1);
    v_setName(name);
    v_init();
    v_setRefresh(60);
}

void platform_frame(void) {
    v_WaitRecal();
    v_readButtons();
    v_readJoystick1Analog();

	if (platform_wait) {
		platform_wait_count++;

		if (platform_wait_count >= DEFAULT_INPUT_WAIT_TIME) {
			platform_wait = false;
		}
	}
}

// MARK: - Input handling

void platform_input_wait(void) {
#if PITREX
	platform_wait = true;
	platform_wait_count = 0;
#endif
}

boolean platform_input_is_left(void) {
	if (platform_wait)
		return false;
		
    if (currentJoy1X < -50) {
		// printf("Left\n");
        return true;
    }
    
    return false;
}

boolean platform_input_is_right(void) {
	if (platform_wait)
		return false;

    if (currentJoy1X > 50) {
    	// printf("Right\n");
    	return true;
    }
    
    return false;
}

boolean platform_input_is_up(void) {
	if (platform_wait)
		return false;

    if (currentJoy1Y > 50) {
		// printf("Up\n");
        return true;
    }
    
    return false;
}

boolean platform_input_is_down(void) {
	if (platform_wait)
		return false;

    if (currentJoy1Y < -50) {
		// printf("Down\n");
        return true;
    }
    
    return false;
}

boolean platform_button_is_pressed(int number) {
	if (platform_wait)
		return false;
    
    switch (number) {
        case 1:
            if ((currentButtonState&0x01) == (0x01))
                return true;
            break;
        case 2:
            if ((currentButtonState&0x02) == (0x02))
                return true;
            break;
        case 3:
            if ((currentButtonState&0x04) == (0x04))
                return true;
            break;
        case 4:
            if ((currentButtonState&0x08) == (0x08))
                return true;
            break;

        default:
            return false;
    }
    
    return false;
}

// MARK: - Drawing

void platform_msg(char* msg, int x, int y, int size, int color) {
	v_printString(x, y, msg, size, color);
}

void platform_raster_msg(char* msg, int x, int y, int size, int color) {
	int tlength = (int)strlen(msg)-1;
	v_printStringRaster(tlength, 0, msg, x, y, '\0');
}

void platform_draw_line(int x1, int y1, int x2, int y2, int color) {
#ifdef PITREX
	int xx1 = MAX_DAC * x1 / 400 - DAC;
	int yy1 = DAC - MAX_DAC * y1 / 400;
	int xx2 = MAX_DAC * x2 / 400 - DAC;
	int yy2 = DAC - MAX_DAC * y2 / 400;

	v_directDraw32(xx1, yy1, xx2, yy2, color);
#else
    v_directDraw32(x1, y1, x2, y2, color);
#endif
}

void platform_draw_lineby(int x1, int y1, int color) {
#ifdef PITREX
    int xx1 = MAX_DAC * x1 / 400;
	int yy1 = MAX_DAC * y1 / 400;

	v_directDeltaDraw32(xx1, yy1, color);
#else
    v_directDeltaDraw32(x1, y1, color);
#endif
}

void platform_moveto(int x1, int y1) {
#ifdef PITREX
    int xx1 = MAX_DAC * x1 / 400 - DAC;
    int yy1 = DAC - MAX_DAC * y1 / 400;

    v_directMove32(xx1, yy1);
#else
    v_directMove32(x1, y1);
#endif
}

void platform_draw_points(int* points, int count, int color) {
    int index = 2;
    int* offset = points;
	
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
        offset++;
        int y2 = *offset;
        offset++;

        platform_draw_line(x1, y1, x2, y2, color);
        
        x1 = x2;
        y1 = y2;
        
        index += 2;
    }
}

void platform_draw_continous_points(int* points, int count, int color) {
    int index = 2;
    int* offset = points;

    if (count < 2)
        return;

    if (count % 2 != 0)
        return;

    int x1 = *offset;
    offset++;
    int y1 = *offset;
    offset++;

    if (count == 2) {
		platform_moveto(x1, y1);
        return;
    }

	platform_moveto(x1, y1);

    while (index < count) {
        x1 = *offset;
        offset++;
        y1 = *offset;
        offset++;

        platform_draw_lineby(x1, y1, color);
        
        index += 2;
    }
}
