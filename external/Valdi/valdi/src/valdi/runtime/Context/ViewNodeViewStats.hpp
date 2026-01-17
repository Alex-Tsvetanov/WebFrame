//
//  ViewNodeViewStats.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/18/19.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class ViewNodeViewStats {
public:
    void addViewClass(const StringBox& viewClass);
    void addLayout();

    const FlatMap<StringBox, size_t>& getNumberOfViewsByViewClass() const;
    size_t getTotalNumberOfViews() const;
    size_t getTotalNumberOfLayouts() const;

private:
    FlatMap<StringBox, size_t> _numbersOfViewsByViewClass;
    size_t _totalNumberOfViews;
    size_t _totalNumberOfLayouts;
};

} // namespace Valdi
