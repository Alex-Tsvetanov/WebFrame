//
//  SCValdiImageFilter.h
//  valdi-ios
//
//  Created by Simon Corsin on 5/2/22.
//

#ifdef __cplusplus

#import <Foundation/Foundation.h>

#import "valdi_core/SCValdiImage.h"
#import "valdi_core/cpp/Attributes/ImageFilter.hpp"

namespace ValdiIOS {

SCValdiImage* postprocessImage(SCValdiImage* input, const Valdi::Ref<Valdi::ImageFilter>& filter);
SCValdiImage* postprocessImage(SCValdiImage* input, const Valdi::Ref<Valdi::ImageFilter>& filter, CIContext* context);

} // namespace ValdiIOS

#endif