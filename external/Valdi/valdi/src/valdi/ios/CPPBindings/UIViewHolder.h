//
//  UIViewHolder.h
//  valdi-ios
//
//  Created by Simon Corsin on 4/26/21.
//

#ifdef __cplusplus

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "valdi/runtime/Views/View.hpp"

#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/SCValdiRef.h"

namespace ValdiIOS {

class UIViewHolder : public Valdi::View, public ObjCObject {
public:
    UIViewHolder(SCValdiRef* viewRef);
    ~UIViewHolder() override;

    snap::valdi_core::Platform getPlatform() const override;
    Valdi::Result<Valdi::Void> rasterInto(const Valdi::Ref<Valdi::IBitmap>& bitmap,
                                          const Valdi::Frame& frame,
                                          const Valdi::Matrix& transform,
                                          float rasterScaleX,
                                          float rasterScaleY) override;

    UIView* getView() const;
    SCValdiRef* getRef() const;

    VALDI_CLASS_HEADER(UIViewHolder)

    static UIView* uiViewFromRef(const Valdi::Ref<Valdi::View>& viewRef);
    static Valdi::Ref<UIViewHolder> makeWithUIView(UIView* view);

    static void executeSyncInMainThread(const Valdi::Function<void()>& cb);
};

} // namespace ValdiIOS

#endif