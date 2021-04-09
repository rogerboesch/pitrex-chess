
#import "PlaygroundView.h"

// -----------------------------------------------------------------------------

@interface PlaygroundView ()

@property (nonatomic, retain) NSImageView* renderImageView;
@property (nonatomic) int renderWidth;
@property (nonatomic) int renderHeight;

@end

@implementation PlaygroundView

#pragma mark - Render

- (void)render:(unsigned char*)buffer {
	NSImage *image = [NSImage imageWithBuffer:buffer width:self.renderWidth height:self.renderHeight];
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
    NSLog(@"Key down: %@/%@ (Length: %d)", event.characters, event.charactersIgnoringModifiers, (int)event.characters.length);
}

- (void)keyUp:(NSEvent *)event {
    NSLog(@"Key up: %@/%@ (Length: %d)", event.characters, event.charactersIgnoringModifiers, (int)event.characters.length);
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

// MARK: - Simulated PiTrex calls

int vectrexinit(char viaconfig) {
    return 0;
}

void v_setName(char *name) {}
void v_init(void) {}
void v_setRefresh(int hz) {}

void v_WaitRecal(void) {}
void v_readButtons(void) {}
void v_readJoystick1Analog(void) {}

void v_directMove32(int32_t x1, int32_t y1) {}
void v_directDeltaDraw32(int32_t x1, int32_t y1, uint8_t color) {}

void v_directDraw32(int x1, int y1, int x2, int y2, int color) {}
void v_printString(int x, int y, char* str, int textSize, int color) {}
int  v_printStringRaster(int8_t x, int8_t y, char* str, int8_t xSize, int8_t ySize, unsigned char delimiter) { return 0; }
