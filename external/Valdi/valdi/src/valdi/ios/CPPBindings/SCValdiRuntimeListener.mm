//
//  SCValdiRuntimeListener.m
//  Valdi
//
//  Created by Simon Corsin on 1/8/19.
//

#import "valdi/ios/CPPBindings/SCValdiRuntimeListener.h"

#import "valdi/ios/Utils/ContextUtils.h"
#import "valdi_core/SCValdiActions.h"
#import "valdi/ios/SCValdiContext+CPP.h"
#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiViewModelHolder.h"

#import "valdi_core/SCValdiAction.h"
#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/SCValdiRuntimeManagerProtocol.h"

namespace ValdiIOS
{

SCValdiRuntime *getObjCRuntime(Valdi::Runtime& runtime) {
    auto runtimeListener = dynamic_cast<ValdiIOS::RuntimeListener *>(runtime.getListener().get());
    if (runtimeListener == nullptr) {
        return nil;
    }

    return runtimeListener->getRuntime();
}

RuntimeListener::~RuntimeListener()
{
}

void RuntimeListener::onContextCreated(Valdi::Runtime &runtime, const Valdi::SharedContext &context)
{
    BOOL enableReferenceTracking = getObjCRuntime(runtime).manager.referenceTrackingEnabled;

    SCValdiContext *valdiContext = [[SCValdiContext alloc] initWithContext:context enableReferenceTracking:enableReferenceTracking];
    context->setUserData(ValdiObjectFromNSObject(valdiContext));
    [_runtimeRef makeStrong];
}

void RuntimeListener::onContextDestroyed(Valdi::Runtime &/*runtime*/, Valdi::Context &context)
{
    SCValdiContext *oldValdiContext = getValdiContext(&context);
    context.setUserData(nullptr);

    [oldValdiContext onDestroyed];
}

void RuntimeListener::onContextRendered(Valdi::Runtime &/*runtime*/, const Valdi::SharedContext &context)
{
    SCValdiContext *valdiContext = getValdiContext(context);

    [valdiContext awakeIfNeeded];
    [valdiContext notifyDidRender];
}

void RuntimeListener::onContextLayoutBecameDirty(Valdi::Runtime &/*runtime*/, const Valdi::SharedContext &context)
{
    SCValdiContext *valdiContext = getValdiContext(context);

    [valdiContext notifyLayoutDidBecomeDirty];
}

void RuntimeListener::onAllContextsDestroyed(Valdi::Runtime &/*runtime*/)
{
    [_runtimeRef makeWeak];
}

void RuntimeListener::setRuntime(SCValdiRuntime *runtime)
{
    _runtimeRef = [[SCValdiRef alloc] initWithInstance:runtime strong:NO];
}

SCValdiRuntime *RuntimeListener::getRuntime() const
{
    return _runtimeRef.instance;
}
}
