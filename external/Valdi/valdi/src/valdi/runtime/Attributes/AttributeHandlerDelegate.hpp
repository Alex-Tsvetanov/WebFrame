//
//  AttributeHandlerDelegate.hpp
//  valdi
//
//  Created by Simon Corsin on 8/2/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class Animator;
class View;
class ViewNode;
class ViewTransactionScope;

/**
 The AttributeHandlerDelegate is an interface that abstracts out how an attribute gets
 applied or reset to a ViewNode or its underlying view. The onApply() and onReset() callbacks
 are called whenever an attribute is updated on a ViewNode. These callbacks must perform the
 side effects on the ViewNode and/or its underlying view as a result of the attribute being
 applied or removed from the node.
 */
class AttributeHandlerDelegate : public SimpleRefCountable {
public:
    /**
    Apply the mutations on the ViewNode or view with the given value, optionally animated with the
    provided animator.
     */
    virtual Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                                 ViewNode& viewNode,
                                 const Ref<View>& view,
                                 const StringBox& name,
                                 const Value& value,
                                 const Ref<Animator>& animator) = 0;

    /**
    Reset the mutations that were previously applied on the ViewNode or view.
     */
    virtual void onReset(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& view,
                         const StringBox& name,
                         const Ref<Animator>& animator) = 0;
};

} // namespace Valdi
