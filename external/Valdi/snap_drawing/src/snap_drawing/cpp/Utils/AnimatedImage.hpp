#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Duration.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

#include "valdi_core/cpp/Resources/LoadedAsset.hpp"

#include "valdi_core/cpp/Utils/Result.hpp"

class SkCanvas;

namespace snap::drawing {

#ifdef SNAP_DRAWING_LOTTIE_ENABLED
constexpr bool kLottieEnabled = true;
#else
constexpr bool kLottieEnabled = false;
#endif

class Resources;
class DrawableSurfaceCanvas;
class DrawingContext;
class IFontManager;

class AnimatedImage : public Valdi::LoadedAsset {
public:
    AnimatedImage();
    ~AnimatedImage() override;

    void draw(SkCanvas* canvas,
              const Rect& drawBounds,
              const Duration& time,
              FittingSizeMode fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFit);
    void drawInCanvas(const DrawableSurfaceCanvas& canvas,
                      const Rect& drawBounds,
                      const Duration& time,
                      FittingSizeMode fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFit);

    virtual Duration getCurrentTime() const = 0;
    virtual const Duration& getDuration() const = 0;
    virtual const Size& getSize() const = 0;
    virtual double getFrameRate() const = 0;

    static Valdi::Result<Ref<AnimatedImage>> make(const Ref<IFontManager>& fontManager,
                                                  const Valdi::Byte* data,
                                                  size_t length);

protected:
    virtual void doDraw(SkCanvas* canvas,
                        const Rect& drawBounds,
                        const Duration& time,
                        FittingSizeMode fittingSizeMode) = 0;

private:
    static bool isJsonObject(const Valdi::Byte* data, size_t length);
};

} // namespace snap::drawing
