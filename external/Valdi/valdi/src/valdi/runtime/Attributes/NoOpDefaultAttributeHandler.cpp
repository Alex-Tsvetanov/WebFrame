//
//  NoOpDefaultAttributeHandler.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/16/21.
//

#include "valdi/runtime/Attributes/NoOpDefaultAttributeHandler.hpp"

namespace Valdi {

NoOpDefaultAttributeHandler::NoOpDefaultAttributeHandler() = default;
NoOpDefaultAttributeHandler::~NoOpDefaultAttributeHandler() = default;

Result<Void> NoOpDefaultAttributeHandler::onApply(ViewTransactionScope& /*viewTransactionScope*/,
                                                  ViewNode& /*viewNode*/,
                                                  const Ref<View>& /*view*/,
                                                  const StringBox& /*name*/,
                                                  const Value& /*value*/,
                                                  const Ref<Animator>& /*animator*/) {
    return Void();
}

void NoOpDefaultAttributeHandler::onReset(ViewTransactionScope& /*viewTransactionScope*/,
                                          ViewNode& /*viewNode*/,
                                          const Ref<View>& /*view*/,
                                          const StringBox& /*name*/,
                                          const Ref<Animator>& /*animator*/) {}

} // namespace Valdi
