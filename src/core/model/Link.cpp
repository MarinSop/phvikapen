#include "core/model/Link.hpp"

#include "core/geometry/Rect.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <string_view>

namespace phvikapen::core {
namespace {

// A letter of the alphabet the ways out are written in, made small. Only those letters matter,
// so nothing is asked of the machine's idea of what a letter is.
[[nodiscard]] constexpr char small(char letter) noexcept {
    return letter >= 'A' && letter <= 'Z' ? static_cast<char>(letter - 'A' + 'a') : letter;
}

// The only ways out of the application a link may take: a page over a network, and mail. Anything
// else — a file on this machine, a program to run — is refused, because a notebook can come from
// somebody else.
constexpr std::array<std::string_view, 3> kWaysOut{"http://", "https://", "mailto:"};

}

Link normalized(Link link) {
    link.width = std::max(link.width, Link::kSmallest);
    link.height = std::max(link.height, Link::kSmallest);
    if (link.label.size() > Link::kLongestLabel) {
        link.label.resize(Link::kLongestLabel);
    }
    if (link.where.size() > Link::kLongestWhere) {
        link.where.resize(Link::kLongestWhere);
    }
    return link;
}

bool isSafeToFollow(std::string_view where) noexcept {
    return std::ranges::any_of(kWaysOut, [where](std::string_view way) noexcept {
        return std::ranges::starts_with(
            where, way, [](char here, char there) noexcept { return small(here) == there; });
    });
}

bool goesSomewhere(const Link& link) noexcept {
    if (link.kind == LinkKind::Page) {
        return !link.page.isNil();
    }
    return isSafeToFollow(link.where);
}

const PlacedLink* linkUnder(std::span<const PlacedLink> links, Point spot) noexcept {
    const PlacedLink* found = nullptr;
    for (const PlacedLink& placed : links) {
        const Rect area = areaOf(placed.link);
        if (spot.x < area.left || spot.x > area.right || spot.y < area.top
            || spot.y > area.bottom) {
            continue;
        }
        if (found == nullptr || placed.ordinal > found->ordinal) {
            found = &placed;
        }
    }
    return found;
}

}
