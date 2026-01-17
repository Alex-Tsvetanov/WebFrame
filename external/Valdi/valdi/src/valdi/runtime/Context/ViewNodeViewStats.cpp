//
//  ViewNodeViewStats.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/18/19.
//

#include "valdi/runtime/Context/ViewNodeViewStats.hpp"

namespace Valdi {

void ViewNodeViewStats::addViewClass(const StringBox& viewClass) {
    const auto& it = _numbersOfViewsByViewClass.find(viewClass);
    if (it == _numbersOfViewsByViewClass.end()) {
        _numbersOfViewsByViewClass[viewClass] = 1;
    } else {
        _numbersOfViewsByViewClass[viewClass] = it->second + 1;
    }
    _totalNumberOfViews++;
}

void ViewNodeViewStats::addLayout() {
    _totalNumberOfLayouts++;
}

const FlatMap<StringBox, size_t>& ViewNodeViewStats::getNumberOfViewsByViewClass() const {
    return _numbersOfViewsByViewClass;
}

size_t ViewNodeViewStats::getTotalNumberOfLayouts() const {
    return _totalNumberOfLayouts;
}

size_t ViewNodeViewStats::getTotalNumberOfViews() const {
    return _totalNumberOfViews;
}

} // namespace Valdi
