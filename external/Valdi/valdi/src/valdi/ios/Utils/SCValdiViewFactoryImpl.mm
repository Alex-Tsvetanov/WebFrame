//
//  SCValdiViewFactoryImpl.m
//  valdi-ios
//
//  Created by Simon Corsin on 4/12/21.
//

#import "SCValdiViewFactoryImpl.h"
#import "valdi_core/SCValdiMarshallableObject.h"

@interface SCValdiViewFactory: NSObject<SCValdiMarshallableObject>
@end

@implementation SCValdiViewFactory

+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor
{
    return SCValdiMarshallableObjectDescriptorMake(nil, nil, nil, SCValdiMarshallableObjectTypeUntyped);
}

@end

@implementation SCValdiViewFactoryImpl

@end
