//
//  NoOpDefaultAttributeHandler.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/16/21.
//

#pragma once

#include "valdi/runtime/Attributes/Animator.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"

namespace Valdi {

class NoOpDefaultAttributeHandler : public AttributeHandlerDelegate {
public:
    NoOpDefaultAttributeHandler();
    ~NoOpDefaultAttributeHandler() override;

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& view,
                         const StringBox& name,
                         const Value& value,
                         const Ref<Animator>& animator) override;
    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& view,
                 const StringBox& name,
                 const Ref<Animator>& animator) override;
};

} // namespace Valdi
