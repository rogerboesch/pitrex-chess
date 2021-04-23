
#ifndef platform_h
#define platform_h

#ifdef __cplusplus
extern "C" {
#endif

// MARK: - Platform types

#ifndef boolean
    #define boolean int
    #define true 1
    #define false 0
#endif

#define P_PI 3.14159265358979323846264338327950288
#define DEG_TO_RAD(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define RAD_TO_DEG(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

// MARK: - Platform

void platform_init(char* name, int width, int height, int hz);
void platform_frame(void);

// MARK: - Input handling

#define BUTTON_ONE      1
#define BUTTON_TWO      2
#define BUTTON_THREE    3
#define BUTTON_FOUR     4

boolean platform_input_is_left(void);
boolean platform_input_is_right(void);
boolean platform_input_is_up(void);
boolean platform_input_is_down(void);
boolean platform_button_is_pressed(int number);
boolean platform_get_control_state(int code);
void platform_set_control_state(int code, boolean press);

// MARK: - Remote input handling
void platform_start_remote_input(void);

// MARK: - Drawing

#define DEFAULT_COLOR           80
#define HIGHLIGHT_COLOR         127
#define LOWLIGHT_COLOR          60
#define DEFAULT_TEXT_SIZE       7
#define DEFAULT_TEXT_SMALL_SIZE 6
#define DEFAULT_INPUT_WAIT_TIME 30

void platform_draw_line(int x1, int y1, int x2, int y2, int color);
void platform_draw_lines(int* points, int count, int color);
void platform_msg(char* msg, int x, int y, int size, int color);
void platform_raster_msg(char* msg, int x, int y, int size, int color);

// MARK: - OS
double platform_get_ms(void);
const char* platform_bundle_file_path(const char* filename, const char* extension);

#ifdef __cplusplus
}
#endif

#endif
