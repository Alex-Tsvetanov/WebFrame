//
//  VelocityTracker.cpp
//  valdi-skia
//
//  Created by Vincent Brunet on 02/16/21.
//

#include "snap_drawing/cpp/Utils/VelocityTracker.hpp"

#include <cmath>

namespace snap::drawing {

const Scalar kApproxSqrt2 = 1.41421356237f;
const size_t kMomentHistory = 10;

VelocityTracker::VelocityTracker() = default;

VelocityTracker::Moment::Moment(const TimePoint& time, Scalar sample) : time(time), sample(sample) {}

void VelocityTracker::addSample(const TimePoint& time, Scalar sample) {
    // First element in the list is the most recent event
    _moments.emplace_front(time, sample);
    while (_moments.size() > kMomentHistory) {
        _moments.pop_back();
    }
}

Scalar VelocityTracker::computeVelocity() const {
    return computeImpulseVelocity();
}

void VelocityTracker::clear() {
    _moments.clear();
}

Scalar VelocityTracker::computeImpulseVelocity() const {
    auto count = _moments.size();
    // If 0 or 1 points, velocity is zero
    if (count < 2) {
        return 0;
    }
    // If 2 points, basic linear calculation
    if (count == 2) {
        auto moment0 = _moments[0];
        auto moment1 = _moments[1];
        Scalar timeDiff = (moment1.time - moment0.time).seconds();
        if (timeDiff == 0) {
            return 0;
        }
        return (moment1.sample - moment0.sample) / timeDiff;
    }
    // Guaranteed to have at least 3 points here
    Scalar work = 0;
    // Start with the oldest sample and go forward in time
    for (size_t i = count - 1; i > 0; i--) {
        auto momentCurrent = _moments[i];
        auto momentNext = _moments[i - 1];
        // Events have identical time stamps, skipping sample
        if (momentCurrent.time == momentNext.time) {
            continue;
        }
        Scalar timeDiff = (momentCurrent.time - momentNext.time).seconds();
        Scalar velocityPrev = VelocityTracker::kineticEnergyToVelocity(work);
        Scalar velocityCurrent = (momentCurrent.sample - momentNext.sample) / timeDiff;
        work += (velocityCurrent - velocityPrev) * fabsf(velocityCurrent);
        if (i == count - 1) {
            work *= 0.5f; // initial condition
        }
    }
    return kineticEnergyToVelocity(work);
}

Scalar VelocityTracker::kineticEnergyToVelocity(Scalar work) {
    return (work < 0 ? -1.0f : 1.0f) * sqrtf(fabsf(work)) * kApproxSqrt2;
}

} // namespace snap::drawing
