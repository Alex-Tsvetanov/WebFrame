//
//  StandaloneNodeRef.hpp
//  valdi-standalone-pc
//
//  Created by Simon Corsin on 10/11/2022
//

#pragma once

#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

class ViewNode;

class StandaloneNodeRef : public ValdiObject {
public:
    StandaloneNodeRef(const Ref<ViewNode>& viewNode, bool isInPlatformReference);
    ~StandaloneNodeRef() override;

    const Ref<ViewNode>& getNode() const;
    bool isInPlatformReference() const;

    VALDI_CLASS_HEADER(StandaloneNodeRef)

private:
    Ref<ViewNode> _viewNode;
    bool _isInPlatformReference;
};

} // namespace Valdi
