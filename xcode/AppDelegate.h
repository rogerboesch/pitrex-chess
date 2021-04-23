//
//  AppDelegate.h
//  PitrexPlayground
//
//  Created by Roger Boesch on 09.04.21.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

+ (void)setName:(char *)name;
+ (void)renderPlayground:(NSImage *)image lines:(int)lines;

@end

