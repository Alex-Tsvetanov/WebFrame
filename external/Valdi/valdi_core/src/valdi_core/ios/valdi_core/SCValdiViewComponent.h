//
//  SCValdiViewComponent.h
//  Valdi
//
//  Created by Simon Corsin on 8/21/18.
//

#import <Foundation/Foundation.h>

@protocol SCValdiContextProtocol;
@protocol SCValdiViewNodeProtocol;

@protocol SCValdiViewComponent <NSObject>

- (void)didMoveToValdiContext:(id<SCValdiContextProtocol>)valdiContext viewNode:(id<SCValdiViewNodeProtocol>)viewNode;

@end
