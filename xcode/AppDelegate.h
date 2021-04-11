//
//  AppDelegate.h
//  PitrexPlayground
//
//  Created by Roger Boesch on 09.04.21.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

+ (void)setName:(char *)name;
+ (void)setSize:(int)width height:(int)height;
+ (void)renderPlayground:(NSImage *)image;

@end

