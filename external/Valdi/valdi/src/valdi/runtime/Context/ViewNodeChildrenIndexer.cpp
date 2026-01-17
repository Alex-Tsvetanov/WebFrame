//
//  ViewNodeChildrenIndexer.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 12/6/19.
//

#include "valdi/runtime/Context/ViewNodeChildrenIndexer.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace Valdi {

ChildrenVisibilityResult::ChildrenVisibilityResult()
    : visibleChildren(makeReusableArray<ViewNode*>()), invisibleChildren(makeReusableArray<ViewNode*>()) {}

ViewNodeChildrenIndexer::ViewNodeChildrenIndexer(ViewNode* viewNode) : _viewNode(viewNode) {}

void ViewNodeChildrenIndexer::setNeedsUpdate() {
    _needUpdate = true;
}

bool ViewNodeChildrenIndexer::needsUpdate() const {
    return _needUpdate;
}

void ViewNodeChildrenIndexer::setHorizontal(bool horizontal) {
    if (_horizontal != horizontal) {
        _horizontal = horizontal;
        setNeedsUpdate();
    }
}

void ViewNodeChildrenIndexer::updateVisibleBounds(const Frame& viewport) {
    if (_cellSize == 0.0f) {
        _visibleLowerBound = 0;
        _visibleUpperBound = 0;
        return;
    }

    float start;
    float end;
    if (_horizontal) {
        start = viewport.getLeft();
        end = viewport.getRight();
    } else {
        start = viewport.getTop();
        end = viewport.getBottom();
    }

    _visibleUpperBound = std::min(static_cast<size_t>(std::ceil(end / _cellSize)), _cells.size());
    _visibleLowerBound = std::min(static_cast<size_t>(std::max(start / _cellSize, 0.0f)), _visibleUpperBound);
}

void appendNodeIfNeeded(std::vector<ViewNode*>& output, ViewNode* viewNode, int updateId) {
    if (viewNode->getLastChildrenIndexerId() != updateId) {
        viewNode->setLastChildrenIndexerId(updateId);
        output.emplace_back(viewNode);
    }
}

void ViewNodeChildrenIndexer::appendNodesIfNeeded(std::vector<ViewNode*>& output,
                                                  size_t from,
                                                  size_t to,
                                                  int updateId) {
    auto begin = _cells.begin() + from;
    auto end = _cells.begin() + to;
    while (begin != end) {
        for (auto* node : *begin) {
            appendNodeIfNeeded(output, node, updateId);
        }
        ++begin;
    }
}

ChildrenVisibilityResult ViewNodeChildrenIndexer::findChildrenVisibility(const Frame& viewport) {
    auto updateId = ++_updateId;
    auto didFullUpdate = _needUpdate;

    if (_needUpdate) {
        _needUpdate = false;
        rebuild();
    }

    auto lastVisibleLowerBound = _visibleLowerBound;
    auto lastVisibleUpperBound = _visibleUpperBound;

    updateVisibleBounds(viewport);

    ChildrenVisibilityResult result;

    appendNodesIfNeeded(*result.visibleChildren, _visibleLowerBound, _visibleUpperBound, updateId);

    if (didFullUpdate) {
        // On full update, we append all the invisible nodes in the output since the nodes
        // may have moved or been inserted since the last findChildrenVisibility() call.

        appendNodesIfNeeded(*result.invisibleChildren, 0, _visibleLowerBound, updateId);
        appendNodesIfNeeded(*result.invisibleChildren, _visibleUpperBound, _cells.size(), updateId);
    } else {
        // On partial update, we figure out which nodes have become invisible.

        if (_visibleLowerBound > lastVisibleLowerBound) {
            // elements at top became invisible
            appendNodesIfNeeded(*result.invisibleChildren, lastVisibleLowerBound, _visibleLowerBound, updateId);
        }

        if (lastVisibleUpperBound > _visibleUpperBound) {
            // elements at become became invisible
            appendNodesIfNeeded(*result.invisibleChildren, _visibleUpperBound, lastVisibleUpperBound, updateId);
        }
    }

    return result;
}

void ViewNodeChildrenIndexer::insertNodeInCells(ViewNode* viewNode, float start, float end) {
    auto cellStart = static_cast<size_t>(start / _cellSize);
    auto cellEnd = static_cast<size_t>(std::ceil(end / _cellSize));

    // Insert in the last cell if the node is outside the range of our container
    cellStart = std::min(cellStart, _cells.size() - 1);

    // Make sure we insert into at least one cell if the node consumes no space
    cellEnd = std::max(cellStart + 1, cellEnd);

    while (cellStart != cellEnd && cellStart < _cells.size()) {
        _cells[cellStart++].emplace_back(viewNode);
    }
}

void ViewNodeChildrenIndexer::rebuild() {
    auto childCount = _viewNode->getChildCount();

    _cells.resize(childCount);
    for (auto& objects : _cells) {
        objects.clear();
    }

    if (_horizontal) {
        if (childCount == 0) {
            _cellSize = _viewNode->getChildrenSpaceWidth();
            return;
        }
        _cellSize = _viewNode->getChildrenSpaceWidth() / static_cast<float>(childCount);

        if (_cellSize != 0.0f) {
            for (auto* child : *_viewNode) {
                auto calculatedOffsetX = child->getDirectionDependentTranslationX();
                insertNodeInCells(child,
                                  child->getCalculatedFrame().getLeft() + calculatedOffsetX,
                                  child->getCalculatedFrame().getRight() + calculatedOffsetX);
            }
        }
    } else {
        if (childCount == 0) {
            _cellSize = _viewNode->getChildrenSpaceHeight();
            return;
        }
        _cellSize = _viewNode->getChildrenSpaceHeight() / static_cast<float>(childCount);

        if (_cellSize != 0.0f) {
            for (auto* child : *_viewNode) {
                auto translationY = child->getTranslationY();
                insertNodeInCells(child,
                                  child->getCalculatedFrame().getTop() + translationY,
                                  child->getCalculatedFrame().getBottom() + translationY);
            }
        }
    }
}

} // namespace Valdi
