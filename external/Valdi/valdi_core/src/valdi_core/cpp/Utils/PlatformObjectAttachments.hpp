//
//  PlatformObjectAttachments.hpp
//  valdi_core
//
//  Created by Simon Corsin on 3/2/23.
//

#pragma once

#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/ValueTypedProxyObject.hpp"

namespace Valdi {

/**
 * Object attached on objects that gets marshalled as proxies,
 * or on objects that got unmarshalled from proxies.
 * Holds the attached proxy either as a strong or weak ref, and
 * keyed by a source pointer.
 */
class PlatformObjectAttachments : public SimpleRefCountable {
public:
    PlatformObjectAttachments();
    ~PlatformObjectAttachments() override;

    Ref<ValueTypedProxyObject> getProxyForSource(RefCountable* source) const;
    void setProxyForSource(RefCountable* source, const Ref<ValueTypedProxyObject>& proxy, bool strongRef);

    Valdi::SmallVector<Ref<ValueTypedProxyObject>, 1> getAllProxies() const;

private:
    struct Attachment {
        Ref<RefCountable> source;
        Weak<ValueTypedProxyObject> weakProxy;
        Ref<ValueTypedProxyObject> strongProxy;

        Ref<ValueTypedProxyObject> getProxy() const;
    };

    Valdi::SmallVector<Attachment, 1> _attachments;

    Attachment& emplaceAttachment(RefCountable* source);
};

} // namespace Valdi
