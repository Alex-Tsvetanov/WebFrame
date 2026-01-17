#pragma once

#include "snap_drawing/cpp/Utils/Color.hpp"
#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"

#include <ostream>
#include <vector>

namespace snap::drawing {

class TestBitmap : public Valdi::IBitmap {
public:
    TestBitmap(int width, int height);

    ~TestBitmap() override;

    void dispose() override;

    Valdi::BitmapInfo getInfo() const override;

    void* lockBytes() override;

    void unlockBytes() override;

    Color getPixel(int x, int y) const;

    void setPixel(int x, int y, Color pixel);

    void setPixels(const std::initializer_list<Color>& pixels);

    void setPixelsRow(int y, const Color* pixels);

    void setOnLockCallback(Valdi::Function<void()>&& onLock);

    bool operator==(const TestBitmap& other) const;
    bool operator!=(const TestBitmap& other) const;
    bool operator==(const std::initializer_list<Color>& pixels) const;
    bool operator!=(const std::initializer_list<Color>& pixels) const;

private:
    Valdi::ByteBuffer _pixels;
    int _width;
    int _height;
    Valdi::Function<void()> _onLock;

    inline size_t getPixelOffset(int x, int y) const;

    inline Color* getPixelPtr(int x, int y) const;
};

class SinglePixelBitmap : public TestBitmap {
public:
    SinglePixelBitmap();

    Color getPixel() const;

    void setPixel(Color pixel);
};

class BitmapBuilder {
public:
    BitmapBuilder(int width, int height);

    BitmapBuilder& row(std::initializer_list<Color> pixels);
    BitmapBuilder& row(const std::vector<Color>& pixels);

    Valdi::Ref<TestBitmap> build();

private:
    int incrementRow();

    int _height;
    int _currentRow = -1;
    Valdi::Ref<TestBitmap> _bitmap;
};

Valdi::Ref<TestBitmap> createTestBitmap();

std::ostream& operator<<(std::ostream& os, const TestBitmap& bitmap);

} // namespace snap::drawing
