#include "valdi/runtime/Context/ContextAutoDestroy.hpp"
#include "valdi/runtime/Context/ContextManager.hpp"

namespace Valdi {

ContextAutoDestroy::ContextAutoDestroy(Valdi::ContextId contextId, Valdi::ContextManager& contextManager)
    : _contextId(contextId), _contextManager(contextManager) {}

ContextAutoDestroy::~ContextAutoDestroy() {
    destroy();
}

void ContextAutoDestroy::destroy() {
    _contextManager.destroyContext(_contextId);
}

Valdi::ContextId ContextAutoDestroy::getContextId() const {
    return _contextId;
}

VALDI_CLASS_IMPL(ContextAutoDestroy)

} // namespace Valdi
