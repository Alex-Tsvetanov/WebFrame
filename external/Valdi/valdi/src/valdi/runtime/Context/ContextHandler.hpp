//
//  ContextHandler.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/17/19.
//

#pragma once

#include "valdi/runtime/Context/RawViewNodeId.hpp"
#include "valdi_core/cpp/Context/ContextId.hpp"
#include "valdi_core/cpp/Utils/ValueConvertible.hpp"

#include <string>

namespace Valdi {

class Context;

class ContextHandler : public SimpleRefCountable {
public:
    ContextHandler() = default;

    /**
     Called whenever the context has been created.
     */
    virtual void onCreate(const Ref<Context>& context,
                          const Shared<ValueConvertible>& viewModel,
                          const Shared<ValueConvertible>& componentContext,
                          ContextUpdateId updateId,
                          bool shouldUpdateSync) = 0;

    /**
     Called whenever the context has been destroyed.
     */
    virtual void onDestroy(const Ref<Context>& context, bool shouldUpdateSync) = 0;

    /**
     Called when the view model of the given contextId has changed.
     */
    virtual void onViewModelUpdate(const Ref<Context>& context,
                                   const Shared<ValueConvertible>& viewModel,
                                   ContextUpdateId updateId,
                                   bool shouldUpdateSync) = 0;

    /**
     Called whenever an attribute for a ViewNode has changed externally
     */
    virtual void onAttributeChange(const Ref<Context>& context,
                                   RawViewNodeId viewNodeId,
                                   const StringBox& attributeName,
                                   const Value& attributeValue,
                                   bool shouldUpdateSync) = 0;

    /**
     Called whenever a runtime issue or error happened.
     The viewNodeId will be non zero if the error relates to a particular node.
     */
    virtual void onRuntimeIssue(const Ref<Context>& context,
                                bool isError,
                                const StringBox& message,
                                bool shouldUpdateSync) = 0;
};

} // namespace Valdi
