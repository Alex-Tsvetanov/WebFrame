//
//  LinkedList.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/15/22.
//

#include "valdi_core/cpp/Utils/LinkedList.hpp"

namespace Valdi {

LinkedListNode::LinkedListNode() = default;

LinkedListNode::~LinkedListNode() {
    remove();
}

LinkedListNode* LinkedListNode::next() const {
    return _next.get();
}

LinkedListNode* LinkedListNode::prev() const {
    return _prev;
}

void LinkedListNode::remove() {
    auto next = std::move(_next);
    auto* prev = _prev;

    _prev = nullptr;

    if (next != nullptr) {
        next->_prev = prev;
    }

    if (prev != nullptr) {
        prev->_next = std::move(next);
    }
}

void LinkedListNode::insert(LinkedListNode* prev, LinkedListNode* next) {
    if (prev != nullptr) {
        prev->_next = this;
    }

    if (next != nullptr) {
        next->_prev = this;
    }

    _prev = prev;
    _next = next;
}

} // namespace Valdi
