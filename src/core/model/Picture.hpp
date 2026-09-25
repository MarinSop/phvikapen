#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"

#include <cstdint>

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
