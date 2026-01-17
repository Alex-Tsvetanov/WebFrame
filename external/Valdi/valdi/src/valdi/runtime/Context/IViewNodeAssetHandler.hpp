//
//  IViewNodeAssetHandler.hpp
//  valdi
//
//  Created by Simon Corsin on 7/25/24.
//

#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class IViewNodeAssetHandler : public SharedPtrRefCountable {
public:
    virtual void onContainerSizeChanged(float width, float height) = 0;
};

} // namespace Valdi
