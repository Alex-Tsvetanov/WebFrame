#include "valdi_core/jni/GlobalRefJavaObject.hpp"

#include "valdi/runtime/Views/ViewFactory.hpp"

namespace ValdiAndroid {

class DeferredViewOperations;
class ViewManager;

class AndroidViewFactory : public Valdi::ViewFactory {
public:
    AndroidViewFactory(GlobalRefJavaObjectBase viewFactory,
                       Valdi::StringBox viewClassName,
                       ViewManager& viewManager,
                       Valdi::Ref<Valdi::BoundAttributes> boundAttributes,
                       bool isUserSpecified);
    ~AndroidViewFactory() override;

protected:
    Valdi::Ref<Valdi::View> doCreateView(Valdi::ViewNodeTree* viewNodeTree, Valdi::ViewNode* viewNode) override;

private:
    ViewManager& _viewManager;
    GlobalRefJavaObjectBase _viewFactory;
};

} // namespace ValdiAndroid
