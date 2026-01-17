//
//  SCValdiMarshallableObjectDescriptor.m
//  valdi-ios
//
//  Created by Simon Corsin on 6/3/20.
//

#import "valdi_core/SCValdiMarshallableObjectDescriptor.h"

SCValdiMarshallableObjectDescriptor SCValdiMarshallableObjectDescriptorMake(const SCValdiMarshallableObjectFieldDescriptor *fields,
                                                                                  const char *const *identifiers,
                                                                                  const SCValdiMarshallableObjectBlockSupport *blocks,
                                                                                  SCValdiMarshallableObjectType objectType) {
    SCValdiMarshallableObjectDescriptor descriptor;
    descriptor.fields = fields;
    descriptor.identifiers = identifiers;
    descriptor.blocks = blocks;
    descriptor.objectType = objectType;
    return descriptor;
}
