
#import "PlaygroundView.h"
#import "AppDelegate.h"
#include "platform.h"

extern int currentButtonState;
extern int currentJoy1X;
extern int currentJoy1Y;

unsigned char* pixel_buffer = NULL;
int render_width = 400;
int render_height = 400;
int line_count = 0;

void fb_init(void);
void fb_clear(void);
void fb_render(void);
void fb_set_pixel(int x, int y, int color);
void fb_draw_line(int x1, int y1, int x2, int y2, int color, int invert);
void fb_draw_line_set_cursor(int x1, int y1, int x2, int y2, int color, int invert);
void fb_moveto(int x, int y);
void fb_lineby(int x, int y, int color, int invert);
void fb_draw_string(int x, int y, char* str, int size, int color);

// MARK: - Simulated PiTrex calls

void v_setName(char *name) { [AppDelegate setName:name]; }
void v_init(void) { fb_init(); }
void v_WaitRecal(void) { fb_render(); usleep(20*1000); fb_clear(); line_count = 0; }
void v_directMove32(int32_t x, int32_t y) { fb_moveto(x, y); }
void v_directDeltaDraw32(int32_t x, int32_t y, uint8_t color) { fb_lineby(x, y, color, 1); line_count++; }
void v_directDraw32(int x1, int y1, int x2, int y2, int color) { fb_draw_line_set_cursor(x1, y1, x2, y2, color, 1); line_count++; }
void v_printString(int x, int y, char* str, int textSize, int color) { fb_draw_string(x, y, str, textSize, color); }

// Unimplemenyed calls (dummies)
int vectrexinit(char viaconfig) { return 0; }
void v_setRefresh(int hz) {}
void v_readButtons(void) {}
void v_readJoystick1Analog(void) {}

int  v_printStringRaster(int8_t x, int8_t y, char* str, int8_t xSize, int8_t ySize, unsigned char delimiter) { return 0; }

@interface PlaygroundView ()

@property (nonatomic, retain) NSImageView* renderImageView;
@property (nonatomic, retain) NSTextField* infoField;

@end

@implementation PlaygroundView

#pragma mark - Render

+ (void)setRenderSize:(int)width height:(int)height {
    render_width = width;
    render_height = height;
    fb_init();
}

- (void)render:(NSImage *)image lines:(int)lines {
    self.renderImageView.image = image;
    self.infoField.stringValue = [NSString stringWithFormat:@"lines: %d", lines];
}

#pragma mark - UI events

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder {
    return YES;
}

- (BOOL)resignFirstResponder {
    return YES;
}

- (void)setInputState:(int)keyCode press:(BOOL)press {
    switch (keyCode) {
        case 123:   // arrrow (left)
            platform_set_control_state(1, press);
            break;
        case 124:   // arrrow (right)
            platform_set_control_state(2, press);
            break;
        case 125:   // arrrow (down)
            platform_set_control_state(3, press);
            break;
        case 126:   // arrrow (up)
            platform_set_control_state(4, press);
            break;
        case 18:   // 1
            platform_set_control_state(5, press);
            break;
        case 19:   // 2
            platform_set_control_state(6, press);
            break;
        case 20:   // 3
            platform_set_control_state(7, press);
            break;
        case 21:   // 4
        case 49:   // 4
            platform_set_control_state(8, press);
            break;
    }
}

- (void)keyDown:(NSEvent *)event {
    [self setInputState:event.keyCode press:true];
}

- (void)keyUp:(NSEvent *)event {
    [self setInputState:event.keyCode press:false];
}

- (void)mouseDown:(NSEvent*)theEvent {
    [self.window makeFirstResponder:self];
}

- (void)forceFirstReponder {
    [self.window makeFirstResponder:self];
}

#pragma mark - View

- (BOOL)isFlipped {
    return true;
}

#pragma mark - Initialisation

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    
    self.wantsLayer = true;
    self.layer.backgroundColor = [NSColor lightGrayColor].CGColor;

    self.renderImageView = [[NSImageView alloc] initWithFrame:frame];
    self.renderImageView.imageScaling = NSImageScaleProportionallyUpOrDown;
    self.renderImageView.imageAlignment = NSImageAlignCenter;
    self.renderImageView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self addSubview:self.renderImageView];

    self.renderImageView.wantsLayer = true;
    self.renderImageView.layer.backgroundColor = [NSColor blackColor].CGColor;

    NSRect rect = NSMakeRect(10, 10, 200, 20);
    self.infoField = [[NSTextField alloc] initWithFrame:rect];
    self.infoField.bezeled = false;
    self.infoField.editable = false;
    self.infoField.backgroundColor = NSColor.clearColor;
    [self addSubview:self.infoField];

    return self;
}

@end

#pragma mark - NSImage extension

@implementation NSImage (Framebuffer)

+ (NSImage *)imageWithBuffer:(void *)buffer width:(int)width height:(int)height {
    size_t bufferLength = width * height * 4;
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, buffer, bufferLength, NULL);
    size_t bitsPerComponent = 8;
    size_t bitsPerPixel = 32;
    size_t bytesPerRow = 4 * width;
    CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast;
    CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;

    CGImageRef iref = CGImageCreate(width,
                                    height,
                                    bitsPerComponent,
                                    bitsPerPixel,
                                    bytesPerRow,
                                    colorSpaceRef,
                                    bitmapInfo,
                                    provider,   // data provider
                                    NULL,       // decode
                                    YES,        // should interpolate
                                    renderingIntent);

    NSImage* image =  [[NSImage alloc] initWithCGImage:iref size:NSMakeSize(width, height)];

    CGImageRelease(iref);
    
    return image;
}

@end

// MARK: - Framebuffer handling

int last_x = 0, last_y = 0;

void fb_init(void) {
    if (pixel_buffer != NULL) {
        free(pixel_buffer);
        pixel_buffer = NULL;
    }
    
    printf("Create render buffer %dx%d\n", render_width, render_height);
    
    pixel_buffer = (unsigned char *)malloc(render_width * render_height * 4);
}

void fb_clear(void) {
    memset(pixel_buffer, 0, render_width * render_height * 4);
}

void fb_render() {
    NSImage *image = [NSImage imageWithBuffer:pixel_buffer width:render_width height:render_height];
    [AppDelegate renderPlayground:image lines:line_count];
}

void fb_set_pixel(int x, int y, int color) {
    if (color >= 0)
        color += 127/2;

    // Is the pixel actually visible?
    if (x >= 0 && x < render_width && y >= 0 && y < render_height) {
        
        int offset = (x + ((render_height - 1) - y) * render_width) * 4;
        
        // Use this to make the origin top-left instead of bottom-right.
        offset = (x + y * render_width) * 4;
        
        if (color == -2) {
            pixel_buffer[offset] = 255;
            pixel_buffer[offset+1] = 0;
            pixel_buffer[offset+2] = 0;
            pixel_buffer[offset+3] = 255;
        }
        else {
            pixel_buffer[offset] = color;
            pixel_buffer[offset+1] = color;
            pixel_buffer[offset+2] = color;
            pixel_buffer[offset+3] = color;
        }
    }
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color, int invert) {
    // Invert y
    if (invert) {
        y1 = render_height-y1;
        y2 = render_height-y2;
    }
    
    int dx = x2 - x1;
    int dy = y2 - y1;
    
    // calculate steps required for generating pixels
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    
    // calculate increment in x & y for each steps
    float xInc = dx / (float) steps;
    float yInc = dy / (float) steps;
    
    float x = x1;
    float y = y1;
    for (int i = 0; i <= steps; i++) {
        fb_set_pixel(x, y, color);
        
        x += xInc;
        y += yInc;
    }
}

void fb_draw_line_set_cursor(int x1, int y1, int x2, int y2, int color, int invert) {
    fb_draw_line(x1, y1, x2, y2, color, invert);
    fb_moveto(x2, y2);
}

void fb_moveto(int x, int y) {
    last_x = x;
    last_y = y;
}

void fb_moveby(int x, int y) {
    last_x += x;
    last_y += y;
}

void fb_lineby(int x, int y, int color, int invert) {
    fb_draw_line(last_x, last_y, last_x+x, last_y+y, color, invert);
    last_x += x;
    last_y += y;
}

// MARK: - Vector font table

#define P(x,y) ((((x) & 0xF) << 4) | (((y) & 0xF) << 0))
#define FONT_UP 0xFE
#define FONT_LAST 0xFF

const unsigned char _vec3x_font_table[128][8] = {
    ['0' - 0x20] = { P(0,0), P(8,0), P(8,12), P(0,12), P(0,0), P(8,12), FONT_LAST },
    ['1' - 0x20] = { P(4,0), P(4,12), P(3,10), FONT_LAST },
    ['2' - 0x20] = { P(0,12), P(8,12), P(8,7), P(0,5), P(0,0), P(8,0), FONT_LAST },
    ['3' - 0x20] = { P(0,12), P(8,12), P(8,0), P(0,0), FONT_UP, P(0,6), P(8,6), FONT_LAST },
    ['4' - 0x20] = { P(0,12), P(0,6), P(8,6), FONT_UP, P(8,12), P(8,0), FONT_LAST },
    ['5' - 0x20] = { P(0,0), P(8,0), P(8,6), P(0,7), P(0,12), P(8,12), FONT_LAST },
    ['6' - 0x20] = { P(0,12), P(0,0), P(8,0), P(8,5), P(0,7), FONT_LAST },
    ['7' - 0x20] = { P(0,12), P(8,12), P(8,6), P(4,0), FONT_LAST },
    ['8' - 0x20] = { P(0,0), P(8,0), P(8,12), P(0,12), P(0,0), FONT_UP, P(0,6), P(8,6), },
    ['9' - 0x20] = { P(8,0), P(8,12), P(0,12), P(0,7), P(8,5), FONT_LAST },
    [' ' - 0x20] = { FONT_LAST },
    ['.' - 0x20] = { P(3,0), P(4,0), FONT_LAST },
    [',' - 0x20] = { P(2,0), P(4,2), FONT_LAST },
    ['-' - 0x20] = { P(2,6), P(6,6), FONT_LAST },
    ['+' - 0x20] = { P(1,6), P(7,6), FONT_UP, P(4,9), P(4,3), FONT_LAST },
    ['!' - 0x20] = { P(4,0), P(3,2), P(5,2), P(4,0), FONT_UP, P(4,4), P(4,12), FONT_LAST },
    ['#' - 0x20] = { P(0,4), P(8,4), P(6,2), P(6,10), P(8,8), P(0,8), P(2,10), P(2,2) },
    ['^' - 0x20] = { P(2,6), P(4,12), P(6,6), FONT_LAST },
    ['=' - 0x20] = { P(1,4), P(7,4), FONT_UP, P(1,8), P(7,8), FONT_LAST },
    ['*' - 0x20] = { P(0,0), P(4,12), P(8,0), P(0,8), P(8,8), P(0,0), FONT_LAST },
    ['_' - 0x20] = { P(0,0), P(8,0), FONT_LAST },
    ['/' - 0x20] = { P(0,0), P(8,12), FONT_LAST },
    ['\\' - 0x20] = { P(0,12), P(8,0), FONT_LAST },
    ['@' - 0x20] = { P(8,4), P(4,0), P(0,4), P(0,8), P(4,12), P(8,8), P(4,4), P(3,6) },
    ['$' - 0x20] = { P(6,2), P(2,6), P(6,10), FONT_UP, P(4,12), P(4,0), FONT_LAST },
    ['&' - 0x20] = { P(8,0), P(4,12), P(8,8), P(0,4), P(4,0), P(8,4), FONT_LAST },
    ['[' - 0x20] = { P(6,0), P(2,0), P(2,12), P(6,12), FONT_LAST },
    [']' - 0x20] = { P(2,0), P(6,0), P(6,12), P(2,12), FONT_LAST },
    ['(' - 0x20] = { P(6,0), P(2,4), P(2,8), P(6,12), FONT_LAST },
    [')' - 0x20] = { P(2,0), P(6,4), P(6,8), P(2,12), FONT_LAST },
    ['{' - 0x20] = { P(6,0), P(4,2), P(4,10), P(6,12), FONT_UP, P(2,6), P(4,6), FONT_LAST },
    ['}' - 0x20] = { P(4,0), P(6,2), P(6,10), P(4,12), FONT_UP, P(6,6), P(8,6), FONT_LAST },
    ['%' - 0x20] = { P(0,0), P(8,12), FONT_UP, P(2,10), P(2,8), FONT_UP, P(6,4), P(6,2) },
    ['<' - 0x20] = { P(6,0), P(2,6), P(6,12), FONT_LAST },
    ['>' - 0x20] = { P(2,0), P(6,6), P(2,12), FONT_LAST },
    ['|' - 0x20] = { P(4,0), P(4,5), FONT_UP, P(4,6), P(4,12), FONT_LAST },
    [':' - 0x20] = { P(4,9), P(4,7), FONT_UP, P(4,5), P(4,3), FONT_LAST },
    [';' - 0x20] = { P(4,9), P(4,7), FONT_UP, P(4,5), P(1,2), FONT_LAST },
    ['"' - 0x20] = { P(2,10), P(2,6), FONT_UP, P(6,10), P(6,6), FONT_LAST },
    ['\'' - 0x20] = { P(2,6), P(6,10), FONT_LAST },
    ['`' - 0x20] = { P(2,10), P(6,6), FONT_LAST },
    ['~' - 0x20] = { P(0,4), P(2,8), P(6,4), P(8,8), FONT_LAST },
    ['?' - 0x20] = { P(0,8), P(4,12), P(8,8), P(4,4), FONT_UP, P(4,1), P(4,0), FONT_LAST },
    ['A' - 0x20] = { P(0,0), P(0,8), P(4,12), P(8,8), P(8,0), FONT_UP, P(0,4), P(8,4) },
    ['B' - 0x20] = { P(0,0), P(0,12), P(4,12), P(8,10), P(4,6), P(8,2), P(4,0), P(0,0) },
    ['C' - 0x20] = { P(8,0), P(0,0), P(0,12), P(8,12), FONT_LAST },
    ['D' - 0x20] = { P(0,0), P(0,12), P(4,12), P(8,8), P(8,4), P(4,0), P(0,0), FONT_LAST },
    ['E' - 0x20] = { P(8,0), P(0,0), P(0,12), P(8,12), FONT_UP, P(0,6), P(6,6), FONT_LAST },
    ['F' - 0x20] = { P(0,0), P(0,12), P(8,12), FONT_UP, P(0,6), P(6,6), FONT_LAST },
    ['G' - 0x20] = { P(6,6), P(8,4), P(8,0), P(0,0), P(0,12), P(8,12), FONT_LAST },
    ['H' - 0x20] = { P(0,0), P(0,12), FONT_UP, P(0,6), P(8,6), FONT_UP, P(8,12), P(8,0) },
    ['I' - 0x20] = { P(0,0), P(8,0), FONT_UP, P(4,0), P(4,12), FONT_UP, P(0,12), P(8,12) },
    ['J' - 0x20] = { P(0,4), P(4,0), P(8,0), P(8,12), FONT_LAST },
    ['K' - 0x20] = { P(0,0), P(0,12), FONT_UP, P(8,12), P(0,6), P(6,0), FONT_LAST },
    ['L' - 0x20] = { P(8,0), P(0,0), P(0,12), FONT_LAST },
    ['M' - 0x20] = { P(0,0), P(0,12), P(4,8), P(8,12), P(8,0), FONT_LAST },
    ['N' - 0x20] = { P(0,0), P(0,12), P(8,0), P(8,12), FONT_LAST },
    ['O' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,0), P(0,0), FONT_LAST },
    ['P' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,6), P(0,5), FONT_LAST },
    ['Q' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,4), P(0,0), FONT_UP, P(4,4), P(8,0) },
    ['R' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,6), P(0,5), FONT_UP, P(4,5), P(8,0) },
    ['S' - 0x20] = { P(0,2), P(2,0), P(8,0), P(8,5), P(0,7), P(0,12), P(6,12), P(8,10) },
    ['T' - 0x20] = { P(0,12), P(8,12), FONT_UP, P(4,12), P(4,0), FONT_LAST },
    ['U' - 0x20] = { P(0,12), P(0,2), P(4,0), P(8,2), P(8,12), FONT_LAST },
    ['V' - 0x20] = { P(0,12), P(4,0), P(8,12), FONT_LAST },
    ['W' - 0x20] = { P(0,12), P(2,0), P(4,4), P(6,0), P(8,12), FONT_LAST },
    ['X' - 0x20] = { P(0,0), P(8,12), FONT_UP, P(0,12), P(8,0), FONT_LAST },
    ['Y' - 0x20] = { P(0,12), P(4,6), P(8,12), FONT_UP, P(4,6), P(4,0), FONT_LAST },
    ['Z' - 0x20] = { P(0,12), P(8,12), P(0,0), P(8,0), FONT_UP, P(2,6), P(6,6), FONT_LAST },

    // Special chars from 65 to 96
    [65] = { P(0,0), P(0,12), P(7,12), P(7,0), P(0,0), FONT_LAST },
    [66] = { P(0,0), P(4,12), P(8,0), P(0,0), FONT_LAST },
    [67] = { P(0,6), P(8,6), P(8,7), P(0,7), FONT_LAST },
};

#define FONT_SCALE_FACTOR   1
#define FONT_WIDTH          10*FONT_SCALE_FACTOR
#define FONT_HEIGHT         16*FONT_SCALE_FACTOR

void _fb_draw_char(char ch, int color) {
    if (ch == '\0')
        ch = ' ';

    const unsigned char* p = _vec3x_font_table[ch-0x20];
    unsigned char bright = 0;
    unsigned char x = 0;
    unsigned char y = 0;
    unsigned char i;
    
    for (i=0; i<8; i++) {
        unsigned char b = *p++;
        
        if (b == FONT_LAST)
            break; // last move
        else if (b == FONT_UP)
            bright = 0; // pen up
        else {
            unsigned char x2 = (b>>4)*FONT_SCALE_FACTOR;
            unsigned char y2 = (b&15)*FONT_SCALE_FACTOR;

            if (bright == 0)
                fb_moveby((char)(x2-x), (char)-(y2-y));
            else
                fb_lineby((char)(x2-x), (char)-(y2-y), color, 0);

            bright = 4;
            x = x2;
            y = y2;
        }
    }
}

void fb_draw_char(int x, int y, char ch, int color) {
    fb_moveto(x, y+FONT_HEIGHT);
    _fb_draw_char(ch, color);
}

void fb_draw_string(int x, int y, char* str, int textSize, int color) {
    y = 256-y;
    x += 127;
    
    while (*str != 0) {
        char ch = *str;
    
        fb_draw_char(x, y, ch, color);
        x += FONT_WIDTH;
        
        str++;
    }
}

void platform_set_size(int width, int height) {
    int size = MAX(width, height);
    [PlaygroundView setRenderSize:size height:size];
}
