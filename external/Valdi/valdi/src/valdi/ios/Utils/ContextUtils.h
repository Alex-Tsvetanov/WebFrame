//
//  ContextUtils.h
//  Valdi
//
//  Created by Simon Corsin on 6/10/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#ifdef __cplusplus

#import "valdi/ios/SCValdiContext.h"

#import "valdi/runtime/Attributes/BoundAttributes.hpp"
#import "valdi/runtime/Context/ViewNodeTree.hpp"

#import <UIKit/UIKit.h>

typedef UIView* (^SCValdiViewFactoryBlock)();

namespace ValdiIOS {

SCValdiContext* getValdiContext(const Valdi::Ref<Valdi::Context>& context);
SCValdiContext* getValdiContext(const Valdi::Shared<Valdi::Context>& context);
SCValdiContext* getValdiContext(const Valdi::Context* context);

Valdi::Ref<Valdi::ViewFactory> createLocalViewFactory(SCValdiViewFactoryBlock viewFactory,
                                                      const Valdi::StringBox& viewClassName,
                                                      Valdi::IViewManager& viewManager,
                                                      const Valdi::Ref<Valdi::BoundAttributes>& boundAttributes);

Valdi::Ref<Valdi::ViewFactory> createGlobalViewFactory(const Valdi::StringBox& viewClassName,
                                                       Valdi::IViewManager& viewManager,
                                                       const Valdi::Ref<Valdi::BoundAttributes>& boundAttributes);

} // namespace ValdiIOS

#endif
