//
//  SCValdiRefUtils.h
//  valdi-ios
//
//  Created by Simon Corsin on 09/20/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class SCValdiRef;
@class UIView;
@protocol SCValdiViewNodeProtocol;

id<SCValdiViewNodeProtocol> _Nullable SCValdiRefGetViewNode(SCValdiRef* ref);
UIView* _Nullable SCValdiRefGetView(SCValdiRef* ref);

NS_ASSUME_NONNULL_END
