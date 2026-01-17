//
//  SCValdiMarshaller+CPP.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/19/19.
//

#import "valdi_core/SCValdiMarshaller.h"
#import "valdi_core/cpp/Utils/Marshaller.hpp"

NS_ASSUME_NONNULL_BEGIN

SCValdiMarshallerRef SCValdiMarshallerWrap(Valdi::Marshaller* marshaller);
Valdi::Marshaller* SCValdiMarshallerUnwrap(SCValdiMarshallerRef marshallerRef);

NS_ASSUME_NONNULL_END
