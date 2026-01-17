#ifdef __cplusplus

#import "SCSnapDrawingUIView.h"

#import "snap_drawing/cpp/Layers/LayerRoot.hpp"
#import "valdi/snap_drawing/Runtime.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface SCSnapDrawingUIView ()

@property (nonatomic, readonly) Valdi::Ref<snap::drawing::LayerRoot> layerRoot;

- (snap::drawing::Runtime*)cppRuntime;

@end

NS_ASSUME_NONNULL_END

#endif