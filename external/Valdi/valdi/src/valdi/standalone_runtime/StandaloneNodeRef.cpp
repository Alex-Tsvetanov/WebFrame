//
//  StandaloneView.cpp
//  valdi-standalone-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#include "valdi/standalone_runtime/StandaloneNodeRef.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"

namespace Valdi {

StandaloneNodeRef::StandaloneNodeRef(const Ref<ViewNode>& viewNode, bool isInPlatformReference)
    : _viewNode(viewNode), _isInPlatformReference(isInPlatformReference) {}

StandaloneNodeRef::~StandaloneNodeRef() = default;

const Ref<ViewNode>& StandaloneNodeRef::getNode() const {
    return _viewNode;
}

bool StandaloneNodeRef::isInPlatformReference() const {
    return _isInPlatformReference;
}

VALDI_CLASS_IMPL(StandaloneNodeRef)

} // namespace Valdi
