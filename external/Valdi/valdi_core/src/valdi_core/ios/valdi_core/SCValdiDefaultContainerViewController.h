#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiContainerViewController.h"

@protocol SCValdiRootViewProtocol;

@interface SCValdiDefaultContainerViewController : UIViewController <SCValdiContainerViewController>

@property (nonatomic, readonly) UIView<SCValdiRootViewProtocol>* valdiView;

@property (nonatomic, assign) BOOL showsNavigationBar;

- (instancetype)initWithValdiView:(UIView<SCValdiRootViewProtocol>*)valdiView;

@end
