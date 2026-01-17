#include "TestBitmap.hpp"
#include "utils/debugging/Assert.hpp"

namespace snap::drawing {

TestBitmap::TestBitmap(int width, int height) : _width(width), _height(height) {
    auto offset = getPixelOffset(0, height);
    _pixels.resize(offset);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            setPixel(x, y, Color::white());
        }
    }
}

TestBitmap::~TestBitmap() = default;

void TestBitmap::dispose() {}

Valdi::BitmapInfo TestBitmap::getInfo() const {
    return Valdi::BitmapInfo(
        _width, _height, Valdi::ColorTypeBGRA8888, Valdi::AlphaTypePremul, static_cast<size_t>(_width) * sizeof(Color));
}

void* TestBitmap::lockBytes() {
    if (_onLock) {
        _onLock();
    }

    return _pixels.data();
}

void TestBitmap::unlockBytes() {}

Color TestBitmap::getPixel(int x, int y) const {
    return *getPixelPtr(x, y);
}

void TestBitmap::setPixel(int x, int y, Color pixel) {
    *getPixelPtr(x, y) = pixel;
}

void TestBitmap::setPixelsRow(int y, const Color* pixels) {
    auto* ptr = getPixelPtr(0, y);

    std::memcpy(ptr, pixels, static_cast<size_t>(_width) * sizeof(Color));
}

void TestBitmap::setOnLockCallback(Valdi::Function<void()>&& onLock) {
    _onLock = std::move(onLock);
}

void TestBitmap::setPixels(const std::initializer_list<Color>& pixels) {
    SC_ASSERT(pixels.size() == static_cast<size_t>(_width) * _height);
    std::memcpy(_pixels.data(), pixels.begin(), pixels.size() * sizeof(Color));
}

bool TestBitmap::operator==(const TestBitmap& other) const {
    if (_width != other._width || _height != other._height) {
        return false;
    }

    return _pixels == other._pixels;
}

bool TestBitmap::operator!=(const TestBitmap& other) const {
    return !(*this == other);
}

bool TestBitmap::operator==(const std::initializer_list<Color>& pixels) const {
    if (pixels.size() != (_pixels.size() / sizeof(Color))) {
        return false;
    }

    const auto* pixelsPtr = getPixelPtr(0, 0);
    for (size_t i = 0; i < pixels.size(); i++) {
        if (pixelsPtr[i] != pixels.begin()[i]) {
            return false;
        }
    }

    return true;
}

bool TestBitmap::operator!=(const std::initializer_list<Color>& pixels) const {
    return !(*this == pixels);
}

inline size_t TestBitmap::getPixelOffset(int x, int y) const {
    return ((y * _width) + x) * sizeof(Color);
}

inline Color* TestBitmap::getPixelPtr(int x, int y) const {
    auto offset = getPixelOffset(x, y);
    return reinterpret_cast<Color*>(_pixels[offset]);
}

SinglePixelBitmap::SinglePixelBitmap() : TestBitmap(1, 1) {}

Color SinglePixelBitmap::getPixel() const {
    return TestBitmap::getPixel(0, 0);
}

void SinglePixelBitmap::setPixel(Color pixel) {
    TestBitmap::setPixel(0, 0, pixel);
}

BitmapBuilder::BitmapBuilder(int width, int height)
    : _height(height), _bitmap(Valdi::makeShared<TestBitmap>(width, height)) {}

int BitmapBuilder::incrementRow() {
    auto row = ++_currentRow;
    SC_ASSERT(row < _height);
    return row;
}

BitmapBuilder& BitmapBuilder::row(std::initializer_list<Color> pixels) {
    _bitmap->setPixelsRow(incrementRow(), pixels.begin());
    return *this;
}

BitmapBuilder& BitmapBuilder::row(const std::vector<Color>& pixels) {
    _bitmap->setPixelsRow(incrementRow(), pixels.data());
    return *this;
}

Valdi::Ref<TestBitmap> BitmapBuilder::build() {
    SC_ASSERT(_currentRow + 1 == _height);
    return _bitmap;
}

std::ostream& operator<<(std::ostream& os, const TestBitmap& bitmap) {
    auto info = bitmap.getInfo();

    for (int y = 0; y < info.height; y++) {
        os << "\n[";
        for (int x = 0; x < info.width; x++) {
            auto pixel = bitmap.getPixel(x, y);
            if (x > 0) {
                os << ", ";
            }
            os << pixel;
        }
        os << "]";
    }

    return os;
}

Valdi::Ref<TestBitmap> createTestBitmap() {
    const int height = 6;
    BitmapBuilder builder(8, height);
    for (int i = 0; i < height; i++) {
        builder.row({Color(0xFF000000),
                     Color(0x00FF0000),
                     Color(0x0000FF00),
                     Color(0x000000FF),
                     Color(0xFF000000),
                     Color(0x00FF0000),
                     Color(0x0000FF00),
                     Color(0x000000FF)});
    }
    return builder.build();
}

} // namespace snap::drawing
