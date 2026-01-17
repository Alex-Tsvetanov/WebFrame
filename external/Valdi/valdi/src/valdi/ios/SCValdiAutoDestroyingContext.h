#import "valdi_core/SCValdiContextProtocol.h"

/**
 A ValdiContext implementation that bridges to a given Context
 when initialized, and will automatically destroy the context when
 deallocated.
 */
@interface SCValdiAutoDestroyingContext : NSObject <SCValdiContextProtocol>

- (instancetype)initWithContext:(id<SCValdiContextProtocol>)context;

@end
