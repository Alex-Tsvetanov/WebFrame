//
// Created by Carson Holgate on 8/4/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/Layer.hpp"

namespace snap::drawing {

class MassState {
public:
    float value;
    float velocity;
};

class SpringForce {
public:
    SpringForce(double damping, double stiffness, double valueThreshold);
    ~SpringForce() = default;

    void setValueThreshold(double threshold);

    void setFinalPosition(double finalPosition);

    double getFinalPosition() const;

    MassState updateValues(double lastDisplacement, double lastVelocity, long timeElapsed);

    double getAcceleration(double lastDisplacement, double lastVelocity) const;

    bool isAtEquilibrium(double value, double velocity) const;

private:
    double _dampingRatio;
    double _frequency;
    double _valueThreshold;
    double _velocityThreshold;
    double _gammaPlus;
    double _gammaMinus;
    double _dampedFreq;
    double _finalPosition = 1;
    MassState _massState;

    void init();
};

} // namespace snap::drawing
