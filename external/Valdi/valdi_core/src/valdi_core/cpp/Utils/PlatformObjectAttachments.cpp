//
//  PlatformObjectAttachments.cpp
//  valdi_core
//
//  Created by Simon Corsin on 3/2/23.
//

#include "valdi_core/cpp/Utils/PlatformObjectAttachments.hpp"

namespace Valdi {

Ref<ValueTypedProxyObject> PlatformObjectAttachments::Attachment::getProxy() const {
    if (strongProxy != nullptr) {
        return strongProxy;
    } else {
        return Ref<ValueTypedProxyObject>(weakProxy.lock());
    }
}

PlatformObjectAttachments::PlatformObjectAttachments() = default;

PlatformObjectAttachments::~PlatformObjectAttachments() = default;

Ref<ValueTypedProxyObject> PlatformObjectAttachments::getProxyForSource(RefCountable* source) const {
    for (const auto& attachment : _attachments) {
        if (attachment.source.get() == source) {
            return attachment.getProxy();
        }
    }

    return nullptr;
}

void PlatformObjectAttachments::setProxyForSource(RefCountable* source,
                                                  const Ref<ValueTypedProxyObject>& proxy,
                                                  bool strongRef) {
    auto& attachment = emplaceAttachment(source);
    attachment.weakProxy = proxy.toWeak();

    if (strongRef) {
        attachment.strongProxy = proxy;
    } else {
        attachment.strongProxy = nullptr;
    }
}

PlatformObjectAttachments::Attachment& PlatformObjectAttachments::emplaceAttachment(RefCountable* source) {
    for (auto& attachment : _attachments) {
        if (attachment.source.get() == source) {
            return attachment;
        }
    }

    auto& attachment = _attachments.emplace_back();
    attachment.source = Ref<RefCountable>(source);

    return attachment;
}

Valdi::SmallVector<Ref<ValueTypedProxyObject>, 1> PlatformObjectAttachments::getAllProxies() const {
    Valdi::SmallVector<Ref<ValueTypedProxyObject>, 1> output;

    for (const auto& attachment : _attachments) {
        auto proxy = attachment.getProxy();
        if (proxy == nullptr) {
            continue;
        }
        output.emplace_back(std::move(proxy));
    }

    return output;
}

} // namespace Valdi
