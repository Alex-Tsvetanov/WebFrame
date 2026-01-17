//
//  UserSession.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 3/23/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class UserSession : public SharedPtrRefCountable {
public:
    explicit UserSession(const StringBox& userId);
    ~UserSession() override;

    /**
     Returns the userId for the current session.
     */
    const StringBox& getUserId() const;

private:
    StringBox _userId;
};

} // namespace Valdi
