//
//  AppDelegate.m
//  PitrexPlayground
//
//  Created by Roger Boesch on 09.04.21.
//

#import "AppDelegate.h"
#import "PlaygroundView.h"

int chess_main(void);

@interface AppDelegate ()

@property (strong, nonatomic) NSWindow* window;
@property (strong, nonatomic) NSMenu* menu;
@property (strong, nonatomic) PlaygroundView* view;

@end

@implementation AppDelegate

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
    NSRect contentRect = NSMakeRect(5, 110, 1024, 768);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable;

    self.window = [[NSWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:true];
    self.window.title = title;
    self.window.delegate = self;

    self.view = [[PlaygroundView alloc] initWithFrame:self.window.contentView.bounds];
    [self.window.contentView addSubview:self.view];

    [self.window makeKeyAndOrderFront:nil];
}

- (void)createUI:(NSString *)title {
    [self createWindow:title];
    [self createMiniMenu:title];
}

- (void)start {
    chess_main();
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [self createUI:@"Playground"];
    [self performSelectorInBackground:@selector(start) withObject:NULL];
}

@end
