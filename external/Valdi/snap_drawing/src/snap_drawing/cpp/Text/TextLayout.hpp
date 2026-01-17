//
//  TextLayout.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/14/20.
//

#pragma once

#include "snap_drawing/cpp/Text/Font.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Color.hpp"

#include "snap_drawing/cpp/Utils/Geometry.hpp"

#include "include/core/SkTextBlob.h"

#include "valdi_core/cpp/Utils/Value.hpp"

#include <memory>
#include <optional>
#include <string>

namespace snap::drawing {

enum TextAlign {
    TextAlignLeft,
    TextAlignRight,
    TextAlignCenter,
    TextAlignJustify,
};

enum TextDecoration {
    TextDecorationNone,
    TextDecorationStrikethrough,
    TextDecorationUnderline,
};

enum TextOverflow { TextOverflowEllipsis, TextOverflowClip };

struct TextLayoutDecorationEntry {
    Rect bounds;
    /**
     Whether the entry should be drawn before or after the text blobs
     */
    bool predraw;
    std::optional<Color> color;

    TextLayoutDecorationEntry() = default;
    constexpr TextLayoutDecorationEntry(const Rect& bounds, bool predraw, std::optional<Color> color)
        : bounds(bounds), predraw(predraw), color(color) {}
};

struct TextLayoutEntrySegment {
    Rect bounds;
    Ref<Font> font;
    std::string characters;

    TextLayoutEntrySegment();
    TextLayoutEntrySegment(const Rect& bounds, const Ref<Font>& font, const std::string& characters);
};

struct TextLayoutEntry {
    sk_sp<SkTextBlob> textBlob;
    Rect bounds;
    std::optional<Color> color;
    std::vector<TextLayoutEntrySegment> segments;

    TextLayoutEntry();
    TextLayoutEntry(const Rect& bounds, std::optional<Color> color, std::vector<TextLayoutEntrySegment> segments);

    ~TextLayoutEntry();
};

struct TextLayoutAttachment {
    Rect bounds;
    Ref<Valdi::RefCountable> attachment;

    inline TextLayoutAttachment(const Rect& bounds, const Ref<Valdi::RefCountable>& attachment)
        : bounds(bounds), attachment(attachment) {}
};

class TextLayout : public Valdi::SimpleRefCountable {
public:
    TextLayout(Size maxSize,
               std::vector<TextLayoutEntry>&& entries,
               std::vector<TextLayoutDecorationEntry>&& decorations,
               std::vector<TextLayoutAttachment>&& attachments,
               bool fitsInMaxSize);
    ~TextLayout() override;

    const std::vector<TextLayoutEntry>& getEntries() const;
    const std::vector<TextLayoutDecorationEntry>& getDecorations() const;
    const std::vector<TextLayoutAttachment>& getAttachments() const;

    const Size& getMaxSize() const;

    const Rect& getBounds() const;

    bool fitsInMaxSize() const;

    Valdi::Value toJSONValue() const;

    Ref<Valdi::RefCountable> getAttachmentAtPoint(Point location, Scalar tolerance) const;

private:
    Size _maxSize;
    bool _fitsInMaxSize;

    std::vector<TextLayoutEntry> _entries;
    std::vector<TextLayoutDecorationEntry> _decorations;
    std::vector<TextLayoutAttachment> _attachments;
    Rect _bounds = Rect::makeEmpty();
};

} // namespace snap::drawing
