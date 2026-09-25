#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/math/Equation.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace phvikapen::core {

// One piece of type of a drawn equation: what it says, where its top left corner sits, and how
// large it is set, all in the units a page is measured in.
struct Glyph {
    std::string text;
    Point at{};
    float size{};

    friend bool operator==(const Glyph&, const Glyph&) = default;
};

// A line drawn as part of an equation rather than written: the bar of a fraction, or the roof over
// what a root covers.
struct Bar {
    Rect area;

    friend bool operator==(const Bar&, const Bar&) = default;
};

// An equation as it is drawn: every piece of type where it belongs, every line that holds the
// pieces together, and how far the middle of the whole sits below its top, so that one drawing can
// be lined up against another.
struct Drawing {
    std::vector<Glyph> glyphs;
    std::vector<Bar> bars;
    float width{};
    float height{};
    float middle{};

    friend bool operator==(const Drawing&, const Drawing&) = default;
};

// How wide a run of type is when set at a size. Only the window knows this, so it is asked for
// rather than worked out here.
using Measure = std::function<float(std::string_view, float)>;

// How much smaller a raised power is set than what it is raised over, how far it is raised, and the
// room left around the parts of a fraction and under the roof of a root.
inline constexpr float kRaisedShare = 0.68F;
inline constexpr float kSmallestType = 5.0F;
inline constexpr float kLineRoom = 1.2F;

// An equation laid out to be drawn. Division is drawn as a fraction, one part over the other with
// a bar between; a power is set smaller and raised; a root is drawn under a roof. Brackets are put
// back wherever the shape of the equation needs them, whatever the reader typed.
[[nodiscard]] Drawing laidOut(const Equation& equation, float size, const Measure& measure);

// The same drawing carried to another place.
[[nodiscard]] Drawing movedBy(Drawing drawing, float dx, float dy);

}
