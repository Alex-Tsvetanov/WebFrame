//
//  SCValdiRuntimeListener.h
//  Valdi
//
//  Created by Simon Corsin on 1/8/19.
//

#ifdef __cplusplus

#import "valdi/ios/SCValdiRuntime+Private.h"
#import "valdi/runtime/IRuntimeListener.hpp"
#import "valdi_core/SCValdiRef.h"

#import <Foundation/Foundation.h>

namespace ValdiIOS {

class RuntimeListener : public Valdi::IRuntimeListener {
public:
    virtual ~RuntimeListener();

    void onContextCreated(Valdi::Runtime& runtime, const Valdi::SharedContext& context) override;

    void onContextDestroyed(Valdi::Runtime& runtime, Valdi::Context& context) override;

    void onContextRendered(Valdi::Runtime& runtime, const Valdi::SharedContext& context) override;

    void onContextLayoutBecameDirty(Valdi::Runtime& runtime, const Valdi::SharedContext& context) override;

    void onAllContextsDestroyed(Valdi::Runtime& runtime) override;

    void setRuntime(SCValdiRuntime* runtime);
    SCValdiRuntime* getRuntime() const;

private:
    SCValdiRef* _runtimeRef;
};

SCValdiRuntime* getObjCRuntime(Valdi::Runtime& runtime);

} // namespace ValdiIOS

#endif
