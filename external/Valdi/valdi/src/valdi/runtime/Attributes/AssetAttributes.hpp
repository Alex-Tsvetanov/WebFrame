//
//  AssetAttributes.hpp
//  valdi
//
//  Created by Simon Corsin on 8/10/21.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegateWithCallable.hpp"
#include "valdi/runtime/Views/MeasureDelegate.hpp"
#include "valdi_core/AssetOutputType.hpp"

namespace Valdi {

class AssetAttributes {
public:
    AssetAttributes(AttributeIds& attributeIds, snap::valdi_core::AssetOutputType assetOutputType);

    void bind(AttributeHandlerById& attributes, Ref<MeasureDelegate>& outMeasureDelegate);

private:
    AttributeIds& _attributeIds;
    snap::valdi_core::AssetOutputType _assetOutputType;
};

} // namespace Valdi
