
#import <Cocoa/Cocoa.h>


#pragma mark - NSImage extension

@interface NSImage (Framebuffer)

+ (NSImage *)imageWithBuffer:(void *)buffer width:(int)width height:(int)height;

@end

@interface PlaygroundView : NSView {
}

- (void)render:(NSImage *)image;

@end
