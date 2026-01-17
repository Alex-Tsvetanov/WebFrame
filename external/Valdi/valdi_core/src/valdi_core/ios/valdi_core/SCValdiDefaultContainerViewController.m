#import "SCValdiDefaultContainerViewController.h"

#import "valdi_core/SCValdiRootViewProtocol.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/SCValdiINavigator.h"
#import "valdi_core/SCValdiDefaultNavigator.h"

@interface SCValdiDefaultContainerViewController () <SCValdiViewOwner>

@end

@implementation SCValdiDefaultContainerViewController {
    UIView<SCValdiRootViewProtocol> *_valdiView;
    BOOL _showsNavigationBar;
    SCValdiINavigatorPageVisibility _visibility;
    SCValdiINavigatorSetPageVisibilityObserverObserverBlock _visibilityObserver;
}

- (instancetype)initWithValdiView:(UIView<SCValdiRootViewProtocol> *)valdiView
{
    self = [super initWithNibName:nil bundle:nil];
    
    if (self) {
        _showsNavigationBar = NO;
        _valdiView = valdiView;
    }
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationController.navigationBarHidden = !_showsNavigationBar;
    _valdiView.frame = self.view.bounds;
    [self.view addSubview:_valdiView];
}

- (void)viewDidAppear:(BOOL)animated;
{
    [super viewDidAppear:animated];
    
    _visibility = SCValdiINavigatorPageVisibilityVISIBLE;
    [self _notifyVisibility];
    
}

- (void)viewDidDisappear:(BOOL)animated;
{
    [super viewDidDisappear:animated];
    
    _visibility = SCValdiINavigatorPageVisibilityHIDDEN;
    [self _notifyVisibility];
}

- (void)setShowsNavigationBar:(BOOL)showsNavigationBar
{
    _showsNavigationBar = showsNavigationBar;
    self.navigationController.navigationBarHidden = !showsNavigationBar;
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    _valdiView.frame = self.view.bounds;
}

- (void)forceDisableDismissalGesture:(BOOL)disable
{
    self.navigationController.interactivePopGestureRecognizer.enabled = !disable;
}

- (void)setPageVisibilityObserver:(SCValdiINavigatorSetPageVisibilityObserverObserverBlock)observer
{
    _visibilityObserver = observer;
    [self _notifyVisibility];
}

- (void)_notifyVisibility
{
    SCValdiINavigatorSetPageVisibilityObserverObserverBlock observer = _visibilityObserver;
    if (observer) {
        observer(_visibility);
    }
}

@end
