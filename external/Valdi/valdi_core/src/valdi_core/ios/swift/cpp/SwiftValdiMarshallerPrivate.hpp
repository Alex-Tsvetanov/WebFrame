//
//  SwiftValdiMarshallerPrivate.hpp
//  valdi_core
//
//  Created by Edward Lee on 7/09/24.
//

// The purpose of this private header is because Valdi::Marshaller is a C++ class,
// and cannot be included in the header included by Swift code (the public header). These functions
// however still need to be used elsewhere in the C++ codebase, so we use this private header to expose them.

#pragma once

#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include <valdi_core/ios/swift/cpp/SwiftValdiMarshaller.h>

Valdi::Marshaller* SwiftValdiMarshaller_Unwrap(SwiftValdiMarshallerRef marshallerRef);