//
//  main.m
//  PitrexPlayground
//
//  Created by Roger Boesch on 09.04.21.
//

#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        AppDelegate* appDelegate = [[AppDelegate alloc] init];
        
        [app setDelegate:appDelegate];
        
        [app run];
    }
    
    return EXIT_SUCCESS;
}
