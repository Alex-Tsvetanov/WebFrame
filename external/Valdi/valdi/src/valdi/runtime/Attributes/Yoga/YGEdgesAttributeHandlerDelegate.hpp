//
//  YGEdgesAttributeHandlerDelegate.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#pragma once

#include "valdi/runtime/Attributes/Yoga/YogaAttributeHandlerDelegate.hpp"

namespace Valdi {

class YGEdgesBaseAttributeHandlerDelegate : public YogaAttributeHandlerDelegate {
public:
    Result<Void> onApply(YGNodeRef node, const Value& value) override;
    void onReset(YGNodeRef node, YGNodeRef defaultYogaNode) override;

protected:
    virtual void setEdges(YGNodeRef node,
                          const YGCompactValue& top,
                          const YGCompactValue& end,
                          const YGCompactValue& bottom,
                          const YGCompactValue& start) = 0;

private:
    Result<std::vector<YGCompactValue>> parseFlexBoxShorthand(const Value& value);
};

template<typename T>
class YGEdgesAttributeHandlerDelegate : public YGEdgesBaseAttributeHandlerDelegate {
public:
    explicit YGEdgesAttributeHandlerDelegate(T&& getEdges) : _getEdges(std::move(getEdges)) {}

protected:
    void setEdges(YGNodeRef node,
                  const YGCompactValue& top,
                  const YGCompactValue& end,
                  const YGCompactValue& bottom,
                  const YGCompactValue& start) override {
        auto edges = _getEdges(node);
        edges[YGEdgeTop] = top;
        edges[YGEdgeEnd] = end;
        edges[YGEdgeBottom] = bottom;
        edges[YGEdgeStart] = start;
    }

private:
    T _getEdges;
};

} // namespace Valdi
