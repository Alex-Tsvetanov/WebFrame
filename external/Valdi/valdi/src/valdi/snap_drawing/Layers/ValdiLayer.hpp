//
//  ValdiLayer.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 6/30/20.
//

#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "valdi_core/cpp/Utils/ValueConvertible.hpp"

#include <vector>

namespace Valdi {

class Runtime;
class Context;
class ViewManagerContext;

} // namespace Valdi

namespace snap::drawing {

/**
 View acting as the root view for a Valdi view tree.
 */
class ValdiLayer : public Layer {
public:
    ValdiLayer(const Ref<Resources>& resources, Valdi::Runtime* runtime, Valdi::ViewManagerContext* viewManagerContext);
    ~ValdiLayer() override;

    void setComponent(const Valdi::StringBox& componentPath,
                      const Valdi::Value& viewModel,
                      const Valdi::Value& componentContext);

    void setComponent(const Valdi::StringBox& componentPath,
                      const Valdi::Shared<Valdi::ValueConvertible>& viewModel,
                      const Valdi::Shared<Valdi::ValueConvertible>& componentContext);

    void setViewModel(const Valdi::Value& viewModel);

    Size sizeThatFits(Size maxSize) override;

    const Valdi::Ref<Valdi::Context>& getValdiContext() const;

protected:
    void onLayout() override;
    void onBoundsChanged() override;
    void onRightToLeftChanged() override;

private:
    Valdi::Runtime* _runtime;
    Valdi::Ref<Valdi::ViewManagerContext> _viewManagerContext;
    Valdi::Ref<Valdi::Context> _valdiContext;
    Valdi::Ref<Valdi::ValdiObject> _valdiContextUserData;

    void destroyComponent();

    void updateLayoutSpecs();

    Size calculateLayout(Size maxSize);
};

} // namespace snap::drawing
