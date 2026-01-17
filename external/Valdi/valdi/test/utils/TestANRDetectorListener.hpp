#pragma once

#include "valdi/runtime/JavaScript/JavaScriptANRDetector.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"

namespace ValdiTest {

class TestANRDetectorListener : public Valdi::IJavaScriptANRDetectorListener {
public:
    TestANRDetectorListener();
    ~TestANRDetectorListener() override;

    Valdi::JavaScriptANR waitForANR();

    Valdi::JavaScriptANRBehavior onANR(Valdi::JavaScriptANR anr) override;

    std::optional<Valdi::JavaScriptANR> getLastANR();

private:
    Valdi::Mutex _mutex;
    std::optional<Valdi::JavaScriptANR> _anr;
    Valdi::Function<void()> _callback;
};

} // namespace ValdiTest
