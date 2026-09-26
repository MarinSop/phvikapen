#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace phvikapen::core {

enum class LinkKind : std::uint8_t {
    // Another page of this notebook, named by what it is rather than by what it is called, so that
    // renaming it or moving it somewhere else leaves the link pointing at the same page.
    Page,
    // Somewhere outside the application.
    Web,
};

// A patch of a page that takes the reader somewhere else. It stands over whatever is underneath —
// handwriting, type, a picture — rather than belonging to it, so anything at all can be linked.
struct Link {
    static constexpr float kSmallest = 8.0F;
    static constexpr std::size_t kLongestLabel = 200;
    static constexpr std::size_t kLongestWhere = 2000;

    Uuid id;
    Point at{};
    float width{};
    float height{};
    LinkKind kind{LinkKind::Page};
    // The page it goes to, where it goes to a page.
    Uuid page{kNilUuid};
    // Where it goes, where that is outside the application.
    std::string where;
    // What it is called, for a reader who cannot see where it points.
    std::string label;

    friend bool operator==(const Link&, const Link&) = default;
};

struct PlacedLink {
    std::int64_t ordinal{};
    Link link;
    Uuid layer{kNilUuid};

    friend bool operator==(const PlacedLink&, const PlacedLink&) = default;
};

[[nodiscard]] constexpr Rect areaOf(const Link& link) noexcept {
    return Rect{
        .left = link.at.x,
        .top = link.at.y,
        .right = link.at.x + link.width,
        .bottom = link.at.y + link.height,
    };
}

[[nodiscard]] Link normalized(Link link);

// Whether a link has somewhere to go at all. One that points nowhere is never followed.
[[nodiscard]] bool goesSomewhere(const Link& link) noexcept;

// Whether what is written down is somewhere the application is willing to send a reader. Only the
// two ways of reaching a page over a network, and mail, are followed; anything else could run
// something on the machine, so it is refused.
[[nodiscard]] bool isSafeToFollow(std::string_view where) noexcept;

// The link under a point, the topmost one where they overlap, or nothing at all.
[[nodiscard]] const PlacedLink* linkUnder(std::span<const PlacedLink> links, Point spot) noexcept;

}
