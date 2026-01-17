#pragma once

#include "valdi_core/cpp/Context/ContextId.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

class ContextManager;

/**
 * Ref counted object that will automatically destroy the given context from the ContextManager
 * on destruction.
 */
class ContextAutoDestroy : public ValdiObject {
public:
    ContextAutoDestroy(Valdi::ContextId contextId, Valdi::ContextManager& contextManager);
    ~ContextAutoDestroy() override;

    VALDI_CLASS_HEADER(ContextAutoDestroy)

    void destroy();

    Valdi::ContextId getContextId() const;

private:
    Valdi::ContextId _contextId;
    Valdi::ContextManager& _contextManager;
};

} // namespace Valdi
