#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"

#include <cstdint>
#include <string>

namespace phvikapen::core {

// A picture standing on a page: which picture it is, where it stands, how large it is drawn and
// how far it has been turned. What the picture is made of is kept once in the notebook, by what it
// contains, so the same picture on ten pages costs the room of one.
struct Picture {
    static constexpr float kSmallest = 8.0F;
    static constexpr float kDefaultWidth = 320.0F;
    static constexpr float kFullTurn = 360.0F;

    Uuid id;
    ContentId source;
    Point at{};
    float width{kDefaultWidth};
    float height{kDefaultWidth};
    float turn{};

    friend bool operator==(const Picture&, const Picture&) = default;
};

struct PlacedPicture {
    std::int64_t ordinal{};
    Picture picture;
    Uuid layer{kNilUuid};

    friend bool operator==(const PlacedPicture&, const PlacedPicture&) = default;
};

// A run of words read out of a picture, and where it sits within it. The corners are shares of the
// width and height of the picture rather than a place on a page, so that moving the picture,
// drawing it larger or putting the same picture on another page all leave them right.
struct PictureWord {
    std::string text;
    Rect box;

    friend bool operator==(const PictureWord&, const PictureWord&) = default;
};

// The upright box a picture covers, before it is turned.
[[nodiscard]] constexpr Rect areaOf(const Picture& picture) noexcept {
    return Rect{
        .left = picture.at.x,
        .top = picture.at.y,
        .right = picture.at.x + picture.width,
        .bottom = picture.at.y + picture.height,
    };
}

[[nodiscard]] Picture normalized(Picture picture) noexcept;

}
