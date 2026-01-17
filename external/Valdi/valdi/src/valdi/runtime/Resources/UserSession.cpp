//
//  UserSession.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 3/23/20.
//

#include "valdi/runtime/Resources/UserSession.hpp"

namespace Valdi {

UserSession::UserSession(const StringBox& userId) : _userId(userId) {}

UserSession::~UserSession() = default;

const StringBox& UserSession::getUserId() const {
    return _userId;
}

} // namespace Valdi
