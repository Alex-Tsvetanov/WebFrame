//
//  AttributeHandler.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 6/26/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Attributes/Animator.hpp"
#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi/runtime/Attributes/AttributePostprocessor.hpp"
#include "valdi/runtime/Attributes/AttributePreprocessor.hpp"
#include "valdi/runtime/Attributes/CompositeAttribute.hpp"
#include "valdi/runtime/Attributes/PreprocessorCache.hpp"
#include "valdi/runtime/Attributes/Yoga/Yoga.hpp"
#include "valdi/runtime/Views/View.hpp"

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include <deque>
#include <memory>

namespace Valdi {

class ViewNode;
class AttributeHandler;
class PreprocessorCache;
class AttributeHandlerDelegate;

using AttributeHandlerById = FlatMap<AttributeId, AttributeHandler>;

class ViewTransactionScope;

/**
 A handler for a single attribute
 */
class AttributeHandler {
public:
    AttributeHandler();
    AttributeHandler(AttributeId id,
                     StringBox name,
                     const Ref<AttributeHandlerDelegate>& delegate,
                     Ref<CompositeAttribute> compositeAttribute,
                     bool requiresView,
                     bool shouldInvalidateLayoutOnChange);
    AttributeHandler(AttributeId id,
                     StringBox name,
                     Ref<CompositeAttribute> compositeAttribute,
                     bool requiresView,
                     bool shouldInvalidateLayoutOnChange);
    ~AttributeHandler();

    void appendPreprocessor(Result<Value> (*preprocessor)(const Value&), bool isTrivial);
    void appendPreprocessor(AttributePreprocessor preprocessor, bool isTrivial);
    void preprendPreprocessor(Result<Value> (*preprocessor)(const Value&), bool isTrivial);
    void preprendPreprocessor(AttributePreprocessor preprocessor, bool isTrivial);

    void appendPostprocessor(AttributePostprocessor postprocessor);

    [[nodiscard]] Result<PreprocessedValue> preprocess(const Value& value) const;

    [[nodiscard]] Result<Value> preprocessWithoutCache(const Value& value) const;
    Result<Value> postprocess(ViewNode& viewNode, const Value& value) const;

    [[nodiscard]] Result<Void> applyAttribute(ViewTransactionScope& viewTransactionScope,
                                              ViewNode& viewNode,
                                              const Value& value,
                                              const SharedAnimator& animator) const;
    void resetAttribute(ViewTransactionScope& viewTransactionScope,
                        ViewNode& viewNode,
                        const SharedAnimator& animator) const;

    void clearPreprocessorCache();

    /**
     * If this Attribute belongs to a Composite attribute, this will be non null
     */
    const Ref<CompositeAttribute>& getCompositeAttribute() const;

    const StringBox& getName() const;
    AttributeId getId() const;

    /**
     Whether all the preprocessors are trivial and don't need to be cached
     or be lazily processed.
     */
    bool isPreprocessingTrivial() const;

    bool requiresView() const;
    bool shouldInvalidateLayoutOnChange() const;
    bool hasPreprocessors() const;
    bool shouldReevaluateOnColorPaletteChange() const;
    bool isCompositePart() const;

    void setEnablePreprocessorCache(bool enablePreprocessorCache);
    void setShouldReevaluateOnColorPaletteChange(bool reevaluateOnColorPaletteChange);

    AttributeHandler withDelegate(const Ref<AttributeHandlerDelegate>& delegate) const;

    const Ref<AttributeHandlerDelegate>& getDelegate() const;

private:
    AttributeId _id;
    StringBox _name;
    Ref<AttributeHandlerDelegate> _delegate;
    SmallVector<AttributePreprocessor, 1> _preprocessors;
    std::vector<AttributePostprocessor> _postprocessors;
    Ref<CompositeAttribute> _compositeAttribute;
    Shared<PreprocessorCache> _preprocessorCache;
    bool _requiresView = false;
    bool _shouldInvalidateLayoutOnChange = false;
    bool _shouldReevaluateOnColorPaletteChange = false;
    bool _isCompositePart = false;
    bool _preprocessingTrivial = true;
};

} // namespace Valdi
