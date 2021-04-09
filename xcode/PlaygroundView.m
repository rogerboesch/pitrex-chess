
#import "PlaygroundView.h"
#import "AppDelegate.h"

extern int currentButtonState;
extern int currentJoy1X;
extern int currentJoy1Y;

unsigned char* pixel_buffer = NULL;
int render_width = 400;
int render_height = 400;

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))

@interface PlaygroundView ()

@property (nonatomic, retain) NSImageView* renderImageView;

@end

@implementation PlaygroundView

#pragma mark - Render

- (void)render:(NSImage *)image {
	self.renderImageView.image = image;
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

- (void)keyDown:(NSEvent *)event {
    NSLog(@"Key down: %d", event.keyCode);

    switch (event.keyCode) {
        case 123:   // arrrow (left)
            currentJoy1X = -127;
            break;
        case 124:   // arrrow (right)
            currentJoy1X = 127;
            break;
        case 125:   // arrrow (down)
            currentJoy1Y = 127;
            break;
        case 126:   // arrrow (up)
            currentJoy1Y = -127;
            break;
        case 18:   // 1
            BIT_SET(currentButtonState, 0);
            break;
        case 19:   // 2
            BIT_SET(currentButtonState, 1);
            break;
        case 20:   // 3
            BIT_SET(currentButtonState, 2);
            break;
        case 21:   // 4
            BIT_SET(currentButtonState, 3);
            break;
    }
}

- (void)keyUp:(NSEvent *)event {
    NSLog(@"Key up: %d", event.keyCode);
    
    switch (event.keyCode) {
        case 123:   // arrrow (left)
            currentJoy1X = 0;
            break;
        case 124:   // arrrow (right)
            currentJoy1X = 0;
            break;
        case 125:   // arrrow (down)
            currentJoy1Y = 0;
            break;
        case 126:   // arrrow (up)
            currentJoy1Y = 0;
            break;
        case 18:   // 1
            BIT_CLEAR(currentButtonState, 0);
            break;
        case 19:   // 2
            BIT_CLEAR(currentButtonState, 1);
            break;
        case 20:   // 3
            BIT_CLEAR(currentButtonState, 2);
            break;
        case 21:   // 4
            BIT_CLEAR(currentButtonState, 3);
            break;
    }
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
	[self addSubview:self.renderImageView];

	self.renderImageView.wantsLayer = true;
	self.renderImageView.layer.backgroundColor = [NSColor blackColor].CGColor;

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
    
    pixel_buffer = (unsigned char *)malloc(render_width * render_height * 4);
}

void fb_clear(void) {
    memset(pixel_buffer, 0, render_width * render_height * 4);
}

void fb_render() {
    NSImage *image = [NSImage imageWithBuffer:pixel_buffer width:render_width height:render_height];
    [AppDelegate renderPlayground:image];
}

void fb_set_pixel(int x, int y, int color) {
    // Is the pixel actually visible?
    if (x >= 0 && x < render_width && y >= 0 && y < render_height) {
        
        int offset = (x + ((render_height - 1) - y) * render_width) * 4;
        
        // Use this to make the origin top-left instead of bottom-right.
        offset = (x + y * render_width) * 4;
        
        pixel_buffer[offset] = color;
        pixel_buffer[offset+1] = color;
        pixel_buffer[offset+2] = color;
        pixel_buffer[offset+3] = 255;
    }
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color) {
    // Invert y
    y1 = render_height-y1;
    y2 = render_height-y2;

    if (y1 > 400 || y2 > 400) {
        printf("bigger");
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

void fb_moveto(int x, int y) {
    last_x = x;
    last_y = y;
}

void fb_lineby(int x, int y, int color) {
    fb_draw_line(last_x, last_y, last_x+x, last_y+y, color);
    last_x += x;
    last_y += y;
}

// MARK: - Simulated PiTrex calls

void v_setName(char *name) { [AppDelegate setName:name]; }
void v_init(void) { fb_init(); }
void v_WaitRecal(void) { fb_render(); usleep(20*1000); fb_clear(); }
void v_directMove32(int32_t x, int32_t y) { fb_moveto(x, y); }
void v_directDeltaDraw32(int32_t x, int32_t y, uint8_t color) { fb_lineby(x, y, color); }

// Unimplemenyed calls (dummies)
int vectrexinit(char viaconfig) { return 0; }
void v_setRefresh(int hz) {}
void v_readButtons(void) {}
void v_readJoystick1Analog(void) {}

void v_directDraw32(int x1, int y1, int x2, int y2, int color) {}

void v_printString(int x, int y, char* str, int textSize, int color) {}
int  v_printStringRaster(int8_t x, int8_t y, char* str, int8_t xSize, int8_t ySize, unsigned char delimiter) { return 0; }
