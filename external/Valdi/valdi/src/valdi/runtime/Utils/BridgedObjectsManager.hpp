//
//  BridgedObjectsManager.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/11/19.
//

#pragma once

#include <vector>

#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/SequenceIDGenerator.hpp"
#include <optional>
#include <vector>

namespace Valdi {

template<typename T>
class BridgedObject {
public:
    BridgedObject(SequenceID id, T object) : _id(id), _object(std::move(object)) {
        retain();
    }
    BridgedObject() = default;

    const T* getObject() const {
        return &_object;
    }

    void retain() {
        _retainCount++;
    }

    bool release() {
        _retainCount--;
        SC_ASSERT(_retainCount >= 0);

        return _retainCount == 0;
    }

    SequenceID getSequenceID() const {
        return _id;
    }

private:
    SequenceID _id;
    T _object;
    int _retainCount = 0;
};

template<typename T>
class BridgedObjectsManager {
public:
    /**
     Get the bridged object for the given id.
     Returns nullptr if the object is not found
     */
    const T* getObject(uint64_t id) const {
        auto bridgedObject = getBridgedObject(id);
        if (bridgedObject == nullptr) {
            return nullptr;
        }

        return bridgedObject->getObject();
    }

    /**
     Store the object and returns its id.
     The object starts with a reference count
     of 1
     */
    std::optional<uint64_t> storeObject(T object) {
        if (_frozen) {
            return std::nullopt;
        }

        auto sequenceId = _idGenerator.newId();
        expandObjectsIfNeeded(sequenceId.getIndex());

        _objects[sequenceId.getIndex()] = BridgedObject(sequenceId, std::move(object));
        return {sequenceId.getId()};
    }

    std::vector<uint64_t> getAllIds() const {
        std::vector<uint64_t> ids;

        for (const auto& bridgedObject : _objects) {
            if (bridgedObject.getSequenceID().isNull()) {
                continue;
            }
            ids.emplace_back(bridgedObject.getSequenceID().getId());
        }

        return ids;
    }

    void releaseAllObjectsAndFreeze() {
        if (_frozen) {
            return;
        }
        _frozen = true;

        for (auto& bridgedObject : _objects) {
            if (bridgedObject.getSequenceID().isNull()) {
                continue;
            }

            releaseAndDestroyObjectIfNeeded(&bridgedObject);
        }

        _idGenerator.reset();
    }

    bool removeObject(uint64_t id) {
        auto bridgedObject = getBridgedObject(id);
        if (bridgedObject == nullptr) {
            return false;
        }
        destroyBridgedObject(bridgedObject);
        return true;
    }

    bool retainObject(uint64_t id) {
        auto bridgedObject = getBridgedObject(id);
        if (bridgedObject == nullptr) {
            return false;
        }

        bridgedObject->retain();
        return true;
    }

    bool releaseObject(uint64_t id) {
        auto bridgedObject = getBridgedObject(id);
        if (bridgedObject == nullptr) {
            return false;
        }

        releaseAndDestroyObjectIfNeeded(bridgedObject);
        return true;
    }

private:
    std::vector<BridgedObject<T>> _objects;
    SequenceIDGenerator _idGenerator;
    bool _frozen = false;

    void expandObjectsIfNeeded(size_t insertionIndex) {
        if (insertionIndex >= _objects.size()) {
            _objects.reserve(insertionIndex + 1);
            while (_objects.size() <= insertionIndex) {
                _objects.emplace_back();
            }
        }
    }

    void destroyBridgedObject(BridgedObject<T>* bridgedObject) {
        if (!_frozen) {
            _idGenerator.releaseId(bridgedObject->getSequenceID());
        }

        *bridgedObject = BridgedObject<T>();
    }

    bool releaseAndDestroyObjectIfNeeded(BridgedObject<T>* bridgedObject) {
        auto expired = bridgedObject->release();
        if (expired) {
            destroyBridgedObject(bridgedObject);
        }

        return expired;
    }

    std::optional<size_t> getIndexOfId(uint64_t id) const {
        auto parsedId = SequenceID(id);
        if (static_cast<size_t>(parsedId.getIndex()) >= _objects.size()) {
            return std::nullopt;
        }
        auto index = static_cast<size_t>(parsedId.getIndex());
        auto& bridgedObject = _objects[index];
        if (bridgedObject.getSequenceID() != parsedId) {
            return std::nullopt;
        }
        return {index};
    }

    const BridgedObject<T>* getBridgedObject(uint64_t id) const {
        auto objectIndex = getIndexOfId(id);
        if (!objectIndex) {
            return nullptr;
        }
        return &_objects[*objectIndex];
    }

    BridgedObject<T>* getBridgedObject(uint64_t id) {
        auto objectIndex = getIndexOfId(id);
        if (!objectIndex) {
            return nullptr;
        }
        return &_objects[*objectIndex];
    }
};
} // namespace Valdi
