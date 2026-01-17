//
//  Yoga.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/13/18.
//

#pragma once

#include <memory>

struct YGNode;
struct YGConfig;

namespace Valdi {
class ViewNode;

using YGNodeSharedPtr = std::shared_ptr<YGNode>;

class Yoga {
public:
    static std::shared_ptr<YGConfig> createConfig(float pointScale);

    static YGNode* createNode(YGConfig* config);
    static void destroyNode(YGNode* node);

    static void attachViewNode(YGNode* node, ViewNode* viewNode);
    static void detachViewNode(YGNode* node);
    static ViewNode* getAttachedViewNode(YGNode* node);

    static void markNodeDirty(YGNode* node);

    static std::string nodeToString(YGNode* node);
};

} // namespace Valdi
