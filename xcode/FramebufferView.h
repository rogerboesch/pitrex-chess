
#import <Cocoa/Cocoa.h>


#pragma mark - NSImage extension

@interface NSImage (Framebuffer)

+ (NSImage *)imageWithBuffer:(void *)buffer width:(int)width height:(int)height;

@end

@interface FramebufferView : NSView {
}

- (void)render:(unsigned char *)buffer;

@end
