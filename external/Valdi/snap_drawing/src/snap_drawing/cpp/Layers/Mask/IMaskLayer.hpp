//
//  IMaskLayer.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"

namespace snap::drawing {

/**
 MaskLayerPositioning defines where a mask should be prepared/applied within the layer.
 */
enum class MaskLayerPositioning {
    /**
     The mask should be prepared before the background, which will cause
     the masking operation to include the background.
     */
    BelowBackground,

    /**
     The mask should be prepared after the background, which will cause the
     masking operation to NOT include the background.
     */
    AboveBackground

};

/**
 A MaskLayer can set up a mask within a Layer instance. It can be set directly on a Layer, to
 alter how the layer and its children should be drawn.
 */
class IMaskLayer : public Valdi::SimpleRefCountable {
public:
    /**
     Returns the positioning that should be used to prepare the mask within the Layer.
     */
    virtual MaskLayerPositioning getPositioning() = 0;

    /**
     Create the final mask that should be prepared and applied at draw time.
     This method can return null if no masks should be applied.
     */
    virtual Ref<IMask> createMask(const Rect& bounds) = 0;
};

}; // namespace snap::drawing
