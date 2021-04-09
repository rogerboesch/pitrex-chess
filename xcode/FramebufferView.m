
#import "FramebufferView.h"

// -----------------------------------------------------------------------------

@interface FramebufferView ()

@property (nonatomic, retain) NSImageView* renderImageView;
@property (nonatomic) int renderWidth;
@property (nonatomic) int renderHeight;

@end

@implementation FramebufferView

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
