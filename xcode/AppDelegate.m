//
//  AppDelegate.m
//  PitrexPlayground
//
//  Created by Roger Boesch on 09.04.21.
//

#import "AppDelegate.h"
#import "PlaygroundView.h"

const char* platform_bundle_file_path(const char* filename, const char* extension) {
    NSString* path = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:filename] ofType:[NSString stringWithUTF8String:extension]];
    if (path == NULL) {
        return "";
    }

    return [path UTF8String];
}

double platform_get_ms(void) {
    return CFAbsoluteTimeGetCurrent();
}

int game_main(void);

@interface AppDelegate ()

@property (strong, nonatomic) NSWindow* window;
@property (strong, nonatomic) NSMenu* menu;
@property (strong, nonatomic) PlaygroundView* view;

@end

@implementation AppDelegate

// MARK: - Playground helpers

+ (AppDelegate *)sharedDelegate {
    return (AppDelegate *)NSApplication.sharedApplication.delegate;
}

+ (void)setName:(char *)name {
    NSString* title = [NSString stringWithUTF8String:name];

    dispatch_async(dispatch_get_main_queue(), ^{
        NSString* str = [NSString stringWithFormat:@"PiTrex Playground: %@", title];
        [AppDelegate sharedDelegate].window.title = str;
    });
}

+ (void)renderPlayground:(NSImage *)image lines:(int)lines {
    dispatch_async(dispatch_get_main_queue(), ^{
        [[AppDelegate sharedDelegate].view render:image lines:lines];
    });
}

// MARK: - Minimal UI

- (void)createMiniMenu:(NSString *)title {
    self.menu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    NSApp.mainMenu = self.menu;
    
    NSMenuItem *submenu = [self.menu addItemWithTitle:@"Application" action:nil keyEquivalent:@""];
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Application"];
    [self.menu setSubmenu:menu forItem:submenu];

    NSString* str = [NSString stringWithFormat:@"Quit %@", title];
    NSMenuItem *item = [menu addItemWithTitle:str action:@selector(terminate:) keyEquivalent:@"q"];
    item.target = NSApp;
}

- (void)createWindow:(NSString *)title {
    NSRect contentRect = NSMakeRect(5, 110, 800, 600);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable;

    self.window = [[NSWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:true];
    self.window.title = title;
    self.window.delegate = self;

    self.view = [[PlaygroundView alloc] initWithFrame:self.window.contentView.bounds];
    self.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self.window.contentView addSubview:self.view];

    [self.window makeKeyAndOrderFront:nil];
}

- (void)createUI:(NSString *)title {
    [self createWindow:title];
    [self createMiniMenu:title];
}

- (void)start {
    game_main();
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [self createUI:@"Playground"];
    [self performSelectorInBackground:@selector(start) withObject:NULL];
}

@end
