//
//  BoundingBoxHierarchy.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/22/22.
//

#include "snap_drawing/cpp/Utils/BoundingBoxHierarchy.hpp"
#include "include/core/SkBBHFactory.h"

namespace snap::drawing {

BoundingBoxHierarchy::BoundingBoxHierarchy() = default;
BoundingBoxHierarchy::~BoundingBoxHierarchy() = default;

void BoundingBoxHierarchy::insert(const Rect& box) {
    _frames.emplace_back(box.getSkValue());
    _rTree = nullptr;
}

bool BoundingBoxHierarchy::contains(const Rect& box) {
    if (_rTree == nullptr) {
        SkRTreeFactory factory;
        _rTree = factory();
        _rTree->insert(_frames.data(), static_cast<int>(_frames.size()));
    }

    _rTree->search(box.getSkValue(), &_rTreeOutput);
    if (_rTreeOutput.empty()) {
        return false;
    } else {
        _rTreeOutput.clear();
        return true;
    }
}

} // namespace snap::drawing
