//
//  ColorPalette.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/27/20.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace Valdi {

struct Color {
    int64_t value = 0;

    constexpr uint8_t alpha() const {
        return static_cast<uint8_t>(value & 0xFF);
    }

    constexpr double alphaRatio() const {
        return static_cast<double>(alpha()) / 255.0;
    }

    constexpr uint8_t red() const {
        return static_cast<uint8_t>((value >> 24) & 0xFF);
    }

    constexpr uint8_t green() const {
        return static_cast<uint8_t>((value >> 16) & 0xFF);
    }

    constexpr uint8_t blue() const {
        return static_cast<uint8_t>((value >> 8) & 0xFF);
    }

    constexpr Color() = default;
    constexpr Color(int64_t value) : value(value) {}
    constexpr Color(int64_t r, int64_t g, int64_t b, int64_t a)
        : value((r & 0xFF) << 24 | (g & 0xFF) << 16 | (b & 0xFF) << 8 | (a & 0xFF)) {}

    static Color rgba(int64_t r, int64_t g, int64_t b, double a);

    std::string toString() const;

    constexpr bool operator==(const Color& other) const {
        return value == other.value;
    }

    constexpr bool operator!=(const Color& other) const {
        return value != other.value;
    }
};

std::ostream& operator<<(std::ostream& os, const Color& value);

class ColorPalette;

class ColorPaletteListener {
public:
    virtual ~ColorPaletteListener() = default;
    virtual void onColorPaletteUpdated(const ColorPalette& colorPalette) = 0;
};

class ColorPalette : public SharedPtrRefCountable {
public:
    ColorPalette();
    ~ColorPalette() override;

    std::optional<Color> getColorForName(const StringBox& name) const;
    const FlatMap<StringBox, Color>& getColors() const;

    void updateColors(const FlatMap<StringBox, Color>& colors);

    void setListener(ColorPaletteListener* listener);

private:
    FlatMap<StringBox, Color> _colorByName;
    ColorPaletteListener* _listener = nullptr;

    bool setColorForName(const StringBox& name, Color color);
};

} // namespace Valdi
