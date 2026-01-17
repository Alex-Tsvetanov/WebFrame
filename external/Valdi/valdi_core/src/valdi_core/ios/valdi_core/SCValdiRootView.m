#import "valdi_core/SCValdiRootView.h"

#import "valdi_core/SCValdiComponentPath.h"

// Copied from <yoga/YGValue.h> to break valdi_core dependency on yoga
#define YGUndefined NAN

NSNotificationName const SCValdiRootViewDisplayInsetDidChangeNotificationKey = @"SCValdiRootViewDisplayInsetDidChangeNotificationKey";
NSNotificationName const SCValdiRootViewTraitCollectionDidChangeNotificationKey = @"SCValdiRootViewTraitCollectionDidChangeNotificationKey";

@implementation SCValdiRootView

- (instancetype)initWithViewModelUntyped:(id)viewModelUntyped
                 componentContextUntyped:(id)componentContextUntyped
                                 runtime:(id<SCValdiRuntimeProtocol>)runtime
{
    return [self initWithOwner:nil
                     viewModel:viewModelUntyped
              componentContext:componentContextUntyped
                       runtime:runtime];
}

- (instancetype)initWithOwner:(id<SCValdiViewOwner>)owner
                    viewModel:(id)viewModel
             componentContext:(id)componentContext
                      runtime:(id<SCValdiRuntimeProtocol>)runtime
{
    self = [super initWithFrame:CGRectZero];

    NSAssert(runtime, @"Must provide a runtime when initializing a SCValdiRootView");

    if (self) {
        _enableViewInflationWhenInvisible = YES;
        self.owner = owner;

        NSString *viewName = [self viewName];

        BOOL canInflate = viewName.length || [self componentPath];
        NSAssert(canInflate, @"Must have a componentPath to inflate this SCValdiRootView");

        [runtime inflateView:self
                                   owner:owner
                               viewModel:viewModel
                        componentContext:componentContext];
    }

    return self;
}

- (instancetype)initWithOwner:(id<SCValdiViewOwner>)owner
                cppMarshaller:(void*)cppMarshaller
                      runtime:(id<SCValdiRuntimeProtocol>)runtime
{
    self = [super initWithFrame:CGRectZero];

    if (self) {
        _enableViewInflationWhenInvisible = YES;
        self.owner = owner;

        NSString *viewName = [self viewName];

        BOOL canInflate = viewName.length || [self componentPath];
        NSAssert(canInflate, @"Must have a componentPath to inflate this SCValdiRootView");

        [runtime inflateView:self
                          owner:owner
                  cppMarshaller:cppMarshaller];
    }

    return self;
}

- (instancetype)initWithoutValdiContext
{
    self = [super initWithFrame:CGRectZero];

    if (self) {
    }

    return self;
}

- (NSArray *)accessibilityElements
{
    if (self.valdiContext.enableAccessibility && self.valdiViewNode) {
        return @[self.valdiViewNode];
    }
    return [super accessibilityElements];
}

- (BOOL)willEnqueueIntoValdiPool
{
    return NO;
}

- (BOOL)requiresLayoutWhenAnimatingBounds
{
    return NO;
}

- (NSString *)bundleName
{
    NSString *componentPath = self.componentPath;
    if (componentPath) {
        return SCValdiComponentPathGetBundleName(componentPath);
    }
    return nil;
}

- (NSString *)viewName
{
    return nil;
}

- (void)dealloc
{
    if (self.valdiContext.rootValdiViewShouldDestroyContext) {
        [self.valdiContext destroy];
    }
}

- (void)safeAreaInsetsDidChange
{
    [super safeAreaInsetsDidChange];
    [[NSNotificationCenter defaultCenter] postNotificationName:SCValdiRootViewDisplayInsetDidChangeNotificationKey object:self];
}

- (SCValdiLayoutDirection)_valdiLayoutDirection
{
    SCValdiLayoutDirection layoutDirection;
    switch (self.effectiveUserInterfaceLayoutDirection) {
        case UIUserInterfaceLayoutDirectionLeftToRight:
            layoutDirection = SCValdiLayoutDirectionLTR;
            break;
        case UIUserInterfaceLayoutDirectionRightToLeft:
            layoutDirection = SCValdiLayoutDirectionRTL;
            break;
    }
    return layoutDirection;
}

- (void)layoutSubviews
{
    [self updateTraitCollection];
    [super layoutSubviews];
    id<SCValdiContextProtocol> valdiContext = self.valdiContext;
    // The root view takes care of triggering the full FlexBox layout.
    if (valdiContext.rootValdiView == self) {
        [valdiContext setLayoutSize:self.bounds.size direction:[self _valdiLayoutDirection]];
        [self.valdiViewNode didApplyLayoutWithAnimator:nil];
    }
}

- (CGSize)intrinsicContentSize
{
    const CGSize constrainedSize = {
        .width = YGUndefined, .height = YGUndefined,
    };

    return [self sizeThatFits:constrainedSize];
}

- (CGSize)sizeThatFits:(CGSize)size
{
    if (self.valdiContext.rootValdiView != self) {
        return CGSizeMake(0, 0);
    }

    return [self.valdiContext measureLayoutWithMaxSize:size direction:[self _valdiLayoutDirection]];
}

- (void)updateTraitCollection
{
    self.valdiContext.traitCollection = self.traitCollection;
}

- (void)didMoveToWindow
{
    [super didMoveToWindow];

    if (!self.window) {
        // When removing from the window we delay by one frame in case the view is just getting
        // moved around
        __weak typeof(self) weakSelf = self;
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf _updateViewInflationState];
        });
    } else {
        [self _updateViewInflationState];
    }
}

- (void)_updateViewInflationState
{
    self.valdiContext.viewInflationEnabled = (_enableViewInflationWhenInvisible || self.window != nil);
}

- (void)didMoveToValdiContext:(id<SCValdiContextProtocol>)valdiContext
                        viewNode:(id<SCValdiViewNodeProtocol>)viewNode
{
    [self _updateViewInflationState];
}

- (void)waitUntilInitialRenderWithCompletion:(void (^)())completion
{
    [self.valdiContext waitUntilInitialRenderWithCompletion:completion];
}

- (void)onLayoutDirty:(void (^)())onLayoutDirtyCallback
{
    [self.valdiContext onLayoutDirty:onLayoutDirtyCallback];
}

- (void)setVisibleViewportWithFrame:(CGRect)frame
{
    [self.valdiContext setVisibleViewportWithFrame:frame];
}

- (void)unsetVisibleViewport
{
    [self.valdiContext unsetVisibleViewport];
}

- (void)setRetainsLayoutSpecsOnInvalidateLayout:(BOOL)retainsLayoutSpecsOnInvalidateLayout
{
    self.valdiContext.retainsLayoutSpecsOnInvalidateLayout = retainsLayoutSpecsOnInvalidateLayout;
}

- (BOOL)retainsLayoutSpecsOnInvalidateLayout
{
    return self.valdiContext.retainsLayoutSpecsOnInvalidateLayout;
}

- (void)setEnableViewInflationWhenInvisible:(BOOL)enableViewInflationWhenInvisible
{
    if (_enableViewInflationWhenInvisible != enableViewInflationWhenInvisible) {
        _enableViewInflationWhenInvisible = enableViewInflationWhenInvisible;
        [self _updateViewInflationState];
    }
}

// NOTE: Due to the swizzling in UIViewController+DarkMode.h, this will not get called
// if the containing view controller does not set its darkModeEnabled property to YES.
// That would prevent Valdi-based views from picking up the UIUserInterfaceStyle change
// that occurs while the app is running.
//
// IIUC this was introduced so that developers can implement dark mode on a feature-by-feature
// basis, explicitly opting into the UIUserInterfaceStyleDark propagation. We cannot support
// something like that currently in Valdi - if the iOS dark mode tweak is enabled, it's
// enabled for all features.
- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
    [self updateTraitCollection];
    [super traitCollectionDidChange:previousTraitCollection];
    [[NSNotificationCenter defaultCenter] postNotificationName:SCValdiRootViewTraitCollectionDidChangeNotificationKey
                                                        object:self];
}

- (NSString *)componentPath
{
    return [[self class] componentPath];
}

+ (NSString *)componentPath
{
    return nil;
}

- (BOOL)canScrollAtPoint:(CGPoint)point direction:(SCValdiScrollDirection)direction
{
    return [self.valdiViewNode canScrollAtPoint:point direction:direction];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (NSString *)description {
    return [NSString stringWithFormat:
            @"<%@: %p, owner: %@, bundleName: %@, componentPath: %@, enableViewInflationWhenInvisible: %d>",
            NSStringFromClass([self class]), self, self.owner, self.bundleName, self.componentPath, self.enableViewInflationWhenInvisible];
}
#endif

@end
