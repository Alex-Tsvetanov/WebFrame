//
//  SCValdiObjCUtils.h
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 6/28/20.
//

#import <Foundation/Foundation.h>

#import "valdi_core/cpp/Utils/StringBox.hpp"
#import "valdi_core/cpp/Utils/ValdiObject.hpp"
#import "valdi_core/cpp/Utils/Value.hpp"
#import "valdi_core/cpp/Utils/ValueFunction.hpp"

Valdi::StringBox StringFromNSString(__unsafe_unretained NSString* nsString);
NSString* NSStringFromString(const Valdi::StringBox& stringBox);

id NSObjectFromRefCountable(const Valdi::Ref<Valdi::RefCountable>& refCountable);
id NSObjectFromValdiObject(const Valdi::SharedValdiObject& valdiObject);
id NSObjectFromValue(const Valdi::Value& value);
Valdi::Value ValueFromNSObject(id object);

Valdi::SharedValdiObject ValdiObjectFromNSObject(id object, BOOL useStrongRef);
