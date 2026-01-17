#import "valdi/macos/Bootstrap/SCValdiBootstrappingNSAppDelegate.h"
#import "valdi/macos/SCValdiSnapDrawingNSView.h"
#import "valdi/macos/SCValdiRuntime.h"

const NSString *kBootstrappingAppDelegateUseTemporaryCacheDirectoryArgument = @"--use_temporary_caches_directory";

@implementation SCValdiBootstrappingNSAppDelegate {
    NSString *_rootValdiComponentPath;
    NSString *_title;
    int _windowWidth;
    int _windowHeight;
    bool _windowResizable;
    SCValdiRuntime *_valdiRuntime;
    NSWindow *_window;
}

- (instancetype)initWithRootValdiComponentPath:
        (NSString *)rootValdiComponentPath
        title:(NSString *)title
        windowWidth:(int)windowWidth
        windowHeight:(int)windowHeight
        windowResizable:(bool)windowResizable
{
    self = [super init];

    if (self) {
        _rootValdiComponentPath = rootValdiComponentPath;
        _title = title;
        _windowWidth = windowWidth;
        _windowHeight = windowHeight;
        _windowResizable = windowResizable;
    }

    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Configure whether to use the temporary caches directory.
    NSMutableArray *launchArguments = [[SCValdiRuntime getLaunchArguments] mutableCopy];
    BOOL useTemporaryCachesDirectory = [launchArguments containsObject:kBootstrappingAppDelegateUseTemporaryCacheDirectoryArgument];
    _valdiRuntime = [[SCValdiRuntime alloc] initWithUsingTemporaryCachesDirectory:useTemporaryCachesDirectory];

    NSLog(@"Starting %@ for component %@", _title, _rootValdiComponentPath);
    __weak SCValdiBootstrappingNSAppDelegate *weakSelf = self;
    [_valdiRuntime waitUntilReadyWithCompletion:^{
        [weakSelf _onRuntimeReady];
    }];
}

- (void)_onRuntimeReady
{
    SCValdiSnapDrawingNSView *rootView = [[SCValdiSnapDrawingNSView alloc] initWithValdiRuntime:_valdiRuntime arguments:@[] componentContext:nil componentPath:_rootValdiComponentPath];

    int styleMask = NSWindowStyleMaskClosable | NSWindowStyleMaskTitled;
    if (_windowResizable) {
        styleMask |= NSWindowStyleMaskResizable;
    }
    _window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, _windowWidth, _windowHeight) styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    _window.title = _title;
    _window.contentView = rootView;
    [_window makeKeyAndOrderFront:nil];

    [self _setupMainMenu];

    [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

#pragma mark - Private

- (void)_setupMainMenu {
    NSMenu *mainMenu = [[NSMenu alloc] init];
    NSMenuItem *valdiMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:valdiMenuItem];
    NSMenu *valdiMenu = [[NSMenu alloc] init];
    [mainMenu setSubmenu:valdiMenu forItem:valdiMenuItem];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(_quitApplication) keyEquivalent:@"q"];
    [valdiMenu addItem:quitMenuItem];
    NSApplication.sharedApplication.mainMenu = mainMenu;
}

- (void)_quitApplication {
    [NSApp terminate:self];
}

@end
