//
//  SCValdiMacOSStringsBridgeModule.h
//  valdi-macos
//
//  Created by Simon Corsin on 10/14/20.
//

#import "valdi_core/ModuleFactory.hpp"
#import "valdi_core/cpp/Utils/Shared.hpp"
#import <Foundation/Foundation.h>

namespace ValdiMacOS {

class StringsModule : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    StringsModule();
    ~StringsModule() override;

    Valdi::StringBox getModulePath() override;
    Valdi::Value loadModule() override;

private:
    NSMutableArray<NSString*>* _stringTables;

    Valdi::Value getLocalizableString(const Valdi::StringBox& key);
};

} // namespace ValdiMacOS
