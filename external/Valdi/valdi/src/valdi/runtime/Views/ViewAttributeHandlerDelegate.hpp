//
//  ViewAttributeHandlerDelegate.hpp
//  valdi
//
//  Created by Simon Corsin on 7/12/22.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"

namespace Valdi {

class ViewAttributeHandlerDelegate : public AttributeHandlerDelegate {
public:
    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& view,
                         const StringBox& name,
                         const Value& value,
                         const Ref<Animator>& animator) final;

    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& view,
                 const StringBox& name,
                 const Ref<Animator>& animator) final;

protected:
    virtual Result<Void> onViewApply(const Ref<View>& view,
                                     const StringBox& name,
                                     const Value& value,
                                     const Ref<Animator>& animator) = 0;

    virtual void onViewReset(const Ref<View>& view, const StringBox& name, const Ref<Animator>& animator) = 0;
};

} // namespace Valdi
