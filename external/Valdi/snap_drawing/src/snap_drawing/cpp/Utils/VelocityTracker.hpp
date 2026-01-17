//
//  VelocityTracker.hpp
//  valdi-skia
//
//  Created by Vincent Brunet on 02/16/21.
//

#pragma once

#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"

#include <deque>
#include <optional>

namespace snap::drawing {

class VelocityTracker {
public:
    VelocityTracker();

    void addSample(const TimePoint& time, Scalar sample);
    Scalar computeVelocity() const;
    void clear();

private:
    struct Moment {
        TimePoint time;
        Scalar sample;

        Moment(const TimePoint& time, Scalar sample);
    };

    std::deque<Moment> _moments;

    Scalar computeImpulseVelocity() const;

    static Scalar kineticEnergyToVelocity(Scalar work);
};

} // namespace snap::drawing
