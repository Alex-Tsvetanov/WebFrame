//
//  BoundingBoxHierarchy.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/22/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

#include "include/core/SkRefCnt.h"

#include <vector>

class SkBBoxHierarchy;

namespace snap::drawing {

class BoundingBoxHierarchy : public Valdi::SimpleRefCountable {
public:
    BoundingBoxHierarchy();
    ~BoundingBoxHierarchy() override;

    void insert(const Rect& box);
    bool contains(const Rect& box);

private:
    std::vector<SkRect> _frames;
    std::vector<int> _rTreeOutput;
    sk_sp<SkBBoxHierarchy> _rTree;
};

} // namespace snap::drawing
