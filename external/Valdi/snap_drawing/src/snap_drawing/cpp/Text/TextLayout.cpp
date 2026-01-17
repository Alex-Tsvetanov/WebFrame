//
//  TextLayout.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/14/20.
//

#include "snap_drawing/cpp/Text/TextLayout.hpp"
#include "snap_drawing/cpp/Utils/JSONUtils.hpp"

#include "valdi_core/cpp/Utils/ValueArray.hpp"

namespace snap::drawing {

TextLayoutEntrySegment::TextLayoutEntrySegment() = default;
TextLayoutEntrySegment::TextLayoutEntrySegment(const Rect& bounds, const Ref<Font>& font, const std::string& characters)
    : bounds(bounds), font(font), characters(characters) {}

TextLayoutEntry::TextLayoutEntry() = default;
TextLayoutEntry::TextLayoutEntry(const Rect& bounds,
                                 std::optional<Color> color,
                                 std::vector<TextLayoutEntrySegment> segments)
    : bounds(bounds), color(color), segments(std::move(segments)) {}
TextLayoutEntry::~TextLayoutEntry() = default;

TextLayout::TextLayout(Size maxSize,
                       std::vector<TextLayoutEntry>&& entries,
                       std::vector<TextLayoutDecorationEntry>&& decorations,
                       std::vector<TextLayoutAttachment>&& attachments,
                       bool fitsInMaxSize)
    : _maxSize(maxSize),
      _fitsInMaxSize(fitsInMaxSize),
      _entries(std::move(entries)),
      _decorations(std::move(decorations)),
      _attachments(std::move(attachments)) {
    _bounds = Rect::makeEmpty();

    for (const auto& entry : _entries) {
        _bounds.join(entry.bounds);
    }
}

TextLayout::~TextLayout() = default;

const std::vector<TextLayoutEntry>& TextLayout::getEntries() const {
    return _entries;
}

const std::vector<TextLayoutDecorationEntry>& TextLayout::getDecorations() const {
    return _decorations;
}

const std::vector<TextLayoutAttachment>& TextLayout::getAttachments() const {
    return _attachments;
}

const Rect& TextLayout::getBounds() const {
    return _bounds;
}

const Size& TextLayout::getMaxSize() const {
    return _maxSize;
}

bool TextLayout::fitsInMaxSize() const {
    return _fitsInMaxSize;
}

Valdi::Value TextLayout::toJSONValue() const {
    Valdi::Value out;
    out.setMapValue("maxSize", snap::drawing::toJSONValue(_maxSize));

    auto entries = Valdi::ValueArray::make(_entries.size());

    for (size_t i = 0; i < _entries.size(); i++) {
        const auto& entry = _entries[i];

        Valdi::Value entryJson;
        entryJson.setMapValue("bounds", snap::drawing::toJSONValue(entry.bounds));

        if (!entry.segments.empty()) {
            auto segments = Valdi::ValueArray::make(entry.segments.size());
            for (size_t j = 0; j < entry.segments.size(); j++) {
                const auto& segment = entry.segments[j];
                Valdi::Value segmentJson;

                segmentJson.setMapValue("bounds", snap::drawing::toJSONValue(segment.bounds));
                segmentJson.setMapValue("font", Valdi::Value(segment.font->getDescription()));
                segmentJson.setMapValue("characters", Valdi::Value(segment.characters));

                segments->emplace(j, std::move(segmentJson));
            }

            entryJson.setMapValue("segments", Valdi::Value(segments));
        }

        if (entry.color) {
            entryJson.setMapValue("color", Valdi::Value(entry.color.value().toString()));
        }

        entries->emplace(i, std::move(entryJson));
    }

    out.setMapValue("entries", Valdi::Value(entries));

    auto decorations = Valdi::ValueArray::make(_decorations.size());
    for (size_t i = 0; i < _decorations.size(); i++) {
        const auto& decoration = _decorations[i];

        Valdi::Value decorationJson;
        decorationJson.setMapValue("bounds", snap::drawing::toJSONValue(decoration.bounds));
        decorationJson.setMapValue("predraw", Valdi::Value(decoration.predraw));

        if (decoration.color) {
            decorationJson.setMapValue("color", Valdi::Value(decoration.color.value().toString()));
        }

        decorations->emplace(i, std::move(decorationJson));
    }

    out.setMapValue("decorations", Valdi::Value(decorations));

    return out;
}

Ref<Valdi::RefCountable> TextLayout::getAttachmentAtPoint(Point location, Scalar tolerance) const {
    Ref<Valdi::RefCountable> bestCandidate;
    Scalar shortestDistance = 0.0f;
    for (const auto& attachment : _attachments) {
        if (attachment.bounds.contains(location)) {
            return attachment.attachment;
        }

        auto closestPoint = attachment.bounds.closestPoint(location);
        auto distance = Point::distance(location, closestPoint);

        if (distance <= tolerance && (bestCandidate == nullptr || distance < shortestDistance)) {
            shortestDistance = distance;
            bestCandidate = attachment.attachment;
        }
    }

    return bestCandidate;
}

} // namespace snap::drawing
