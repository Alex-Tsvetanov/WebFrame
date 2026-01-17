//
//  SCValdiMacOSStringsBridgeModule.m
//  valdi-macos
//
//  Created by Simon Corsin on 10/14/20.
//

#import "SCValdiMacOSStringsBridgeModule.h"
#import "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#import "valdi/macos/SCValdiObjCUtils.h"

namespace ValdiMacOS {

StringsModule::StringsModule() {
    _stringTables = [NSMutableArray array];
    for (NSString *path in [NSBundle.mainBundle pathsForResourcesOfType:@"strings" inDirectory:nil]) {
        NSString *table = [path.lastPathComponent stringByDeletingPathExtension];
        [_stringTables addObject:table];
    }
}

StringsModule::~StringsModule() = default;

Valdi::Value StringsModule::getLocalizableString(const Valdi::StringBox &key) {
    NSString *localizableKey = NSStringFromString(key);

    for (NSString *stringTable in _stringTables) {
        NSString *localizableString = [NSBundle.mainBundle localizedStringForKey:localizableKey value:nil table:stringTable];
        if (![localizableString isEqualToString:localizableKey]) {
            return Valdi::Value(StringFromNSString(localizableString));
        }
    }

    return Valdi::Value::undefined();
}

Valdi::StringBox StringsModule::getModulePath() {
    return STRING_LITERAL("valdi_core/src/Strings");
}

Valdi::Value StringsModule::loadModule() {
    Valdi::Value module;
    auto strongThis = Valdi::strongSmallRef(this);
    module.setMapValue("getLocalizedString", Valdi::Value(Valdi::makeShared<Valdi::ValueFunctionWithCallable>([strongThis](const Valdi::ValueFunctionCallContext &callContext) -> Valdi::Value {
        auto key = callContext.getParameter(1);
        return strongThis->getLocalizableString(key.toStringBox());
    })));

    return module;
}

}
