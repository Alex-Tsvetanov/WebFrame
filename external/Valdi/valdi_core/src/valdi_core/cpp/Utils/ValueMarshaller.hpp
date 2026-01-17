//
//  ValueMarshaller.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/24/23.
//

#pragma once

#include "valdi_core/cpp/Utils/ExceptionTracker.hpp"
#include "valdi_core/cpp/Utils/PlatformValueDelegate.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"

namespace Valdi {

template<typename ValueType>
class ValueMarshaller : public SimpleRefCountable {
public:
    ValueMarshaller(const Ref<PlatformValueDelegate<ValueType>>& delegate) : _delegate(delegate) {}
    ~ValueMarshaller() override = default;

    virtual bool isOptional() const {
        return false;
    }

    virtual ValueType unmarshall(const Valdi::Value& value,
                                 const ReferenceInfoBuilder& referenceInfoBuilder,
                                 ExceptionTracker& exceptionTracker) = 0;
    virtual Valdi::Value marshall(const ValueType* receiver,
                                  const ValueType& value,
                                  const ReferenceInfoBuilder& referenceInfoBuilder,
                                  ExceptionTracker& exceptionTracker) = 0;

protected:
    Ref<PlatformValueDelegate<ValueType>> _delegate;

    ValueType handleUnmarshallError(ExceptionTracker& exceptionTracker, std::string message) const {
        exceptionTracker.onError(std::move(message));
        return this->_delegate->newNull();
    }

    ValueType onInvalidTypeError(ExceptionTracker& exceptionTracker,
                                 Valdi::ValueType expectedType,
                                 const Valdi::Value& value) {
        exceptionTracker.onError(Valdi::Value::invalidTypeError(expectedType, value.getType()));
        return this->_delegate->newNull();
    }

    Valdi::Value handleMarshallError(ExceptionTracker& exceptionTracker, std::string message) const {
        exceptionTracker.onError(std::move(message));
        return Valdi::Value();
    }

private:
    void setInvalidTypeError(ExceptionTracker& exceptionTracker, ValueType expectedType, const Valdi::Value& value) {
        exceptionTracker.onError(Valdi::Value::invalidTypeError(expectedType, value.getType()));
    }
};

template<typename ValueType>
class ValueMarshallerProcessor : public SimpleRefCountable {
public:
    ~ValueMarshallerProcessor() override = default;

    /**
     * Preprocess the value before converting it into a Valdi::Value
     */
    virtual ValueType preprocess(const ValueType* receiver,
                                 const ValueType& value,
                                 const ReferenceInfoBuilder& referenceInfoBuilder,
                                 ExceptionTracker& exceptionTracker) = 0;

    /**
     * Postprocess the value after it's been converted from a Valdi::Value
     */
    virtual ValueType postprocess(const ValueType& value,
                                  const ReferenceInfoBuilder& referenceInfoBuilder,
                                  ExceptionTracker& exceptionTracker) = 0;
};

} // namespace Valdi
