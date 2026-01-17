#import "valdi/ios/SCValdiAutoDestroyingContext.h"

@implementation SCValdiAutoDestroyingContext {
    NSObject<SCValdiContextProtocol> *_context;
}

- (instancetype)initWithContext:(NSObject<SCValdiContextProtocol>*)context
{
    self = [super init];

    if (self) {
        _context = context;
    }

    return self;
}

- (void)dealloc
{
    [_context destroy];
}

- (SCValdiContextId)contextId
{
    return [_context contextId];
}

- (id<SCValdiRuntimeProtocol>)runtime
{
    return [_context runtime];
}

- (BOOL)enableAccessibility
{
    return _context.enableAccessibility;
}

- (void)setEnableAccessibility:(BOOL)enableAccessibility
{
    _context.enableAccessibility = enableAccessibility;
}

- (BOOL)viewInflationEnabled
{
    return _context.viewInflationEnabled;
}

- (void)setViewInflationEnabled:(BOOL)viewInflationEnabled
{
    _context.viewInflationEnabled = viewInflationEnabled;
}

- (void)setRetainsLayoutSpecsOnInvalidateLayout:(BOOL)retainsLayoutSpecsOnInvalidateLayout
{
    _context.retainsLayoutSpecsOnInvalidateLayout = retainsLayoutSpecsOnInvalidateLayout;
}

- (BOOL)retainsLayoutSpecsOnInvalidateLayout
{
    return _context.retainsLayoutSpecsOnInvalidateLayout;
}

- (BOOL)enableAccurateTouchGesturesInAnimations
{
    return _context.enableAccurateTouchGesturesInAnimations;
}

- (void)setEnableAccurateTouchGesturesInAnimations:(BOOL)enableAccurateTouchGesturesInAnimations
{
    _context.enableAccurateTouchGesturesInAnimations = enableAccurateTouchGesturesInAnimations;
}

- (BOOL)useLegacyMeasureBehavior
{
    return _context.useLegacyMeasureBehavior;
}

- (void)setUseLegacyMeasureBehavior:(BOOL)useLegacyMeasureBehavior
{
    _context.useLegacyMeasureBehavior = useLegacyMeasureBehavior;
}

- (id)viewModel
{
    return _context.viewModel;
}

- (void)setViewModel:(id)viewModel
{
    _context.viewModel = viewModel;
}

- (id)actionHandler
{
    return _context.actionHandler;
}

- (void)setActionHandler:(id)actionHandler
{
    _context.actionHandler = actionHandler;
}

- (id)owner
{
    return _context.owner;
}

- (void)setOwner:(id)owner
{
    _context.owner = owner;
}

- (BOOL)destroyed
{
    return _context.destroyed;
}

- (BOOL)rootValdiViewShouldDestroyContext
{
    return _context.rootValdiViewShouldDestroyContext;
}

- (SCValdiActions *)actions
{
    return _context.actions;
}

- (void)setActions:(SCValdiActions *)actions
{
    _context.actions = actions;
}

- (UITraitCollection *)traitCollection
{
    return _context.traitCollection;
}

- (void)setTraitCollection:(UITraitCollection *)traitCollection
{
    _context.traitCollection = traitCollection;
}

- (id<SCValdiContextProtocol>)rootContext
{
    return _context.rootContext;
}

- (BOOL)hasCompletedInitialRenderIncludingChildComponents
{
    return _context.hasCompletedInitialRenderIncludingChildComponents;
}

- (void)didChangeValue:(id)value
    forValdiAttribute:(NSString*)attributeName
              inViewNode:(id<SCValdiViewNodeProtocol>)viewNode
{
    [_context didChangeValue:value forValdiAttribute:attributeName inViewNode:viewNode];
}

- (void)didChangeValue:(id)value
    forInternedValdiAttribute:(SCValdiInternedStringRef)attributeName
                      inViewNode:(id<SCValdiViewNodeProtocol>)viewNode
{
    [_context didChangeValue:value forInternedValdiAttribute:attributeName inViewNode:viewNode];
}

- (void)destroy
{
    [_context destroy];
}

- (UIView*)rootValdiView
{
    return [_context rootValdiView];
}

- (void)setRootValdiView:(UIView<SCValdiRootViewProtocol>*)rootValdiView
{
    [_context setRootValdiView:rootValdiView];
}

- (id<SCValdiViewNodeProtocol>)rootViewNode
{
    return [_context rootViewNode];
}

- (NSArray<id> *)trackedObjCReferences
{
    return _context.trackedObjCReferences;
}

- (void)setGestureListener:(id<SCValdiGestureListener>)gestureListener
{
    _context.gestureListener = gestureListener;
}

- (id<SCValdiGestureListener>)gestureListener
{
    return _context.gestureListener;
}

- (void)setLayoutSize:(CGSize)size direction:(SCValdiLayoutDirection)direction
{
    [_context setLayoutSize:size direction:direction];
}

- (CGSize)measureLayoutWithMaxSize:(CGSize)size direction:(SCValdiLayoutDirection)direction
{
    return [_context measureLayoutWithMaxSize:size direction:direction];
}

- (void)setVisibleViewportWithFrame:(CGRect)frame
{
    [_context setVisibleViewportWithFrame:frame];
}

- (void)unsetVisibleViewport
{
    [_context unsetVisibleViewport];
}

- (void)waitUntilInitialRenderWithCompletion:(void (^)())completion
{
    [_context waitUntilInitialRenderWithCompletion:completion];
}

- (void)registerViewFactory:(SCValdiViewFactoryBlock)viewFactory forClass:(Class)viewClass
{
    [_context registerViewFactory:viewFactory forClass:viewClass];
}

- (void)registerViewFactory:(SCValdiViewFactoryBlock)viewFactory
           attributesBinder:(SCValdiBindAttributesCallback)attributesBinder
                   forClass:(Class)viewClass
{
    [_context registerViewFactory:viewFactory attributesBinder:attributesBinder forClass:viewClass];
}

- (UIView*)viewForNodeId:(NSString*)nodeId
{
    return [_context viewForNodeId:nodeId];
}

- (void)performJsAction:(NSString*)actionName parameters:(NSArray*)parameters
{
    [_context performJsAction:actionName parameters:parameters];
}

- (void)setParentContext:(id<SCValdiContextProtocol>)parentContext
{
    [_context setParentContext:parentContext];
}

- (void)waitUntilRenderCompletedSync
{
    [_context waitUntilRenderCompletedSync];
}

- (void)waitUntilRenderCompletedSyncWithFlush:(BOOL)shouldFlushRenderRequests
{
    [_context waitUntilRenderCompletedSyncWithFlush:shouldFlushRenderRequests];
}

- (void)waitUntilRenderCompletedWithCompletion:(void (^)())completion
{
    [_context waitUntilRenderCompletedWithCompletion:completion];
}

- (void)onLayoutDirty:(void (^)())onLayoutDirtyCallback
{
    [_context onLayoutDirty:onLayoutDirtyCallback];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (NSString *)description {
    return [NSString stringWithFormat:@"<%@: %p, context: %@>", NSStringFromClass([self class]), self, [_context description]];
}
#endif

@end
