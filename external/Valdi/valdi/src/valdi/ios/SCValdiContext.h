//
//  SCValdiContext.h
//  iOS
//
//  Created by Simon Corsin on 4/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi/ios/SCValdiDisposable.h"
#import "valdi/ios/SCValdiViewNode.h"
#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiContextProtocol.h"
#import "valdi_core/SCValdiInternedString.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class SCValdiRenderedNode;
@class SCValdiContext;
@class SCValdiActions;
@class SCValdiViewNode;
@class SCValdiRuntime;

/**
 A Valdi Context is created for each [UIView viewNamed:owner:] call.
 It is attached to the root view. It contains the underlying Valdi
 document that was used to load the view and the Valdi Node Context,
 which is the unrendered tree.
 */
@interface SCValdiContext : NSObject <SCValdiContextProtocol>

// TODO(simon): Remove once iOS stop using it
@property (readonly, nonatomic) SCValdiContext* rootContext;

@property (strong, nonatomic) id viewModel;

@property (weak, nonatomic) id actionHandler;

@property (weak, nonatomic) id owner;

@property (strong, nonatomic) SCValdiActions* actions;

@property (strong, nonatomic) UITraitCollection* traitCollection;

@property (readonly, nonatomic) UIView* rootValdiView;

@property (readonly, nonatomic) uint32_t objectID;

@property (readonly, nonatomic) id<SCValdiRuntimeProtocol> runtime;

@property (readonly, nonatomic) NSString* componentPath;

@property (readonly, nonatomic) NSString* moduleOwnerName;

@property (readonly, nonatomic) NSString* moduleName;

/**
 * Enables Valdi-managed accessibility element tree
 */
@property (assign, nonatomic) BOOL enableAccessibility;

/**
  Allows to turn on/off view inflation.
  When off, only the root view is kept, all the child views
  will be removed from the tree.
 */
@property (assign, nonatomic) BOOL viewInflationEnabled;

/** returns YES if this view has rendered a view model at least once, or has none, and all of its child components also
 * have. */
// TODO(simon): Remove once iOS stop using it
@property (nonatomic, readonly) BOOL hasCompletedInitialRenderIncludingChildComponents;

@property (assign, nonatomic) BOOL rootValdiViewShouldDestroyContext;

@property (assign, nonatomic) BOOL useLegacyMeasureBehavior;

VALDI_NO_INIT

- (void)destroy;

- (void)awakeIfNeeded;

- (UIView*)viewForNodeId:(NSString*)nodeId;

- (void)notifyDidRender;

- (void)notifyLayoutDidBecomeDirty;

/**
 Add a disposable, which will be disposed whenever the Valdi is destroyed.
 */
- (void)addDisposable:(id<SCValdiDisposable>)disposable;

/**
 Attach an opaque object to this ValdiContext.
 */
- (void)setAttachedObject:(id)attachedObject forKey:(NSString*)key;

/**
 Get the opaque object which was attached for the given key.
 */
- (id)attachedObjectForKey:(NSString*)key;

/**
 Notify the runtime that a value has changed for the given attribute.
 This should be called only for attributes that can be mutated outside of Valdi.
 For example the text value of an editable text view.
 */
- (void)didChangeValue:(id)value forValdiAttribute:(NSString*)attributeName inViewNode:(SCValdiViewNode*)viewNode;

- (void)didChangeValue:(id)value
    forInternedValdiAttribute:(SCValdiInternedStringRef)attributeName
                   inViewNode:(SCValdiViewNode*)viewNode;

- (void)setViewModelNoUpdate:(id)viewModel;

- (void)waitUntilInitialRenderWithCompletion:(void (^)())completion;

- (void)performJsAction:(NSString*)actionName parameters:(NSArray*)parameters;

/**
 Register a custom view factory for the given view class. The created views from
 this factory will be enqueued into a local view pool instead of the global view pool.
 */
- (void)registerViewFactory:(SCValdiViewFactoryBlock)viewFactory forClass:(Class)viewClass;

/**
 Register a custom view factory for the given view class. The created views from
 this factory will be enqueued into a local view pool instead of the global view pool.
 If provided, the attributesBinder callback will be used to bind any additional attributes.
 */
- (void)registerViewFactory:(SCValdiViewFactoryBlock)viewFactory
           attributesBinder:(SCValdiBindAttributesCallback)attributesBinder
                   forClass:(Class)viewClass;

/**
 Attaches this context to the given parent context.
 */
- (void)setParentContext:(id<SCValdiContextProtocol>)parentContext;

/**
 Get the current ValdiContext instance.
 Will be only available as part of call of JS to Objective-C
 */
+ (SCValdiContext*)currentContext;

@end
