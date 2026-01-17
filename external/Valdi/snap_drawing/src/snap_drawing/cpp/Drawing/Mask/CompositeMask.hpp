#pragma once

#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Paint.hpp"

#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"

namespace snap::drawing {

class CompositeMask : public IMask {
public:
    explicit CompositeMask(std::vector<Valdi::Ref<IMask>>&& masks);
    ~CompositeMask() override;

    Rect getBounds() const override;

    void prepare(SkCanvas* canvas) override;

    void apply(SkCanvas* canvas) override;

    String getDescription() override;

private:
    std::vector<Valdi::Ref<IMask>> _masks;
    Rect _bounds;
};

} // namespace snap::drawing
