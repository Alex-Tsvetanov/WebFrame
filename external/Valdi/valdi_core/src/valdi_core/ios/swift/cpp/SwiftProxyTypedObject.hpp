#pragma once
#include "valdi_core/cpp/Utils/ValueTypedProxyObject.hpp"
#include "valdi_core/ios/swift/cpp/SwiftUtils.hpp"

namespace ValdiSwift {

class SwiftProxyObject : public Valdi::ValueTypedProxyObject {
public:
    SwiftProxyObject(const Valdi::Ref<Valdi::ValueTypedObject>& typedObject, const void* object)
        : Valdi::ValueTypedProxyObject(typedObject), _swiftObject(object) {
        retainSwiftObject(_swiftObject);
    }

    ~SwiftProxyObject() override {
        releaseSwiftObject(_swiftObject);
    }

    std::string_view getType() const final {
        return "Swift Proxy";
    }

    const void* getValue() const {
        return _swiftObject;
    }

private:
    const void* _swiftObject;
};

} // namespace ValdiSwift
