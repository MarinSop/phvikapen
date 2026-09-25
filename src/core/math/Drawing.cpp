#include "core/math/Drawing.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/math/Answer.hpp"
#include "core/math/Equation.hpp"
#include "core/model/TextBox.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr std::string_view kPlus = "+";
constexpr std::string_view kMinus = "−";
constexpr std::string_view kTimes = "×";
constexpr std::string_view kRootSign = "√";
constexpr std::string_view kOpen = "(";
constexpr std::string_view kClose = ")";

// The room left around what is drawn: on either side of a sign, above and below the bar of a
// fraction, and between the root sign and what it covers.
constexpr float kSignRoom = 0.22F;
constexpr float kFractionRoom = 0.16F;
constexpr float kBarThickness = 0.06F;
constexpr float kRoofRoom = 0.12F;
constexpr float kRootWidth = 0.55F;
// How far down from the top of what is raised over the foot of a power sits.
constexpr float kShoulderShare = 0.5F;
// Halfway, for the middle of a line and the middle of a bar.
constexpr float kHalfway = 0.5F;

// How tightly what is drawn binds, so that brackets are put back only where the shape needs them.
constexpr int kBindsLoosest = 1;
constexpr int kBindsProduct = 2;
constexpr int kBindsTightest = 4;

[[nodiscard]] int bindingOf(const Equation& equation, std::size_t at) {
    if (at >= equation.steps.size()) {
        return kBindsTightest;
    }
    const Equation::Step& step = equation.steps[at];
    if (step.kind == Equation::Kind::Number) {
        return kBindsTightest;
    }
    switch (step.operation) {
    case Operation::Add:
    case Operation::Subtract:
        return kBindsLoosest;
    case Operation::Multiply:
        return kBindsProduct;
    case Operation::Negate:
        return kBindsLoosest;
    default:
        // A fraction, a power and a root all hold themselves together by how they are drawn.
        return kBindsTightest;
    }
}

[[nodiscard]] std::string_view signOf(Operation operation) {
    switch (operation) {
    case Operation::Add:
        return kPlus;
    case Operation::Multiply:
        return kTimes;
    default:
        return kMinus;
    }
}

class Draughtsman {
public:
    Draughtsman(const Equation& equation, const Measure& measure) noexcept
        : m_equation{&equation}, m_measure{&measure} {}

    [[nodiscard]] Drawing of(std::size_t at, float size, int deep) {
        if (deep > Equation::kDeepest || at >= m_equation->steps.size()) {
            return {};
        }
        const Equation::Step& step = m_equation->steps[at];
        if (step.kind == Equation::Kind::Number) {
            return wordOf(writtenAnswer(step.number), size);
        }
        switch (step.operation) {
        case Operation::Divide:
            return fraction(step, size, deep);
        case Operation::Power:
            return power(step, size, deep);
        case Operation::Root:
            return root(step, size, deep);
        case Operation::Negate:
            return withSign(step, size, deep);
        default:
            return beside(step, at, size, deep);
        }
    }

    // One run of type on its own, which is as tall as a line of that size.
    [[nodiscard]] Drawing wordOf(const std::string& said, float size) const {
        const float wide = (*m_measure)(said, size);
        const float tall = pageUnitsOfPoints(size) * kLineRoom;
        return Drawing{
            .glyphs = {Glyph{.text = said, .at = Point{}, .size = size}},
            .bars = {},
            .width = wide,
            .height = tall,
            .middle = tall * kHalfway,
        };
    }

private:
    // Two drawings set side by side and lined up on their middles.
    [[nodiscard]] static Drawing joinedUp(const Drawing& left, const Drawing& right, float gap) {
        const float middle = std::max(left.middle, right.middle);
        const float under = std::max(left.height - left.middle, right.height - right.middle);
        Drawing put = movedBy(left, 0.0F, middle - left.middle);
        const Drawing after = movedBy(right, left.width + gap, middle - right.middle);
        put.glyphs.insert(put.glyphs.end(), after.glyphs.begin(), after.glyphs.end());
        put.bars.insert(put.bars.end(), after.bars.begin(), after.bars.end());
        put.width = left.width + gap + right.width;
        put.height = middle + under;
        put.middle = middle;
        return put;
    }

    // Brackets grow with what they hold, so that a pair around a fraction reaches from its top to
    // its foot rather than sitting small beside it.
    [[nodiscard]] Drawing bracketed(const Drawing& inside, float size) const {
        const float line = pageUnitsOfPoints(size) * kLineRoom;
        const float grown =
            line > 0.0F && inside.height > line ? size * (inside.height / line) : size;
        const Drawing open = wordOf(std::string{kOpen}, grown);
        const Drawing close = wordOf(std::string{kClose}, grown);
        return joinedUp(joinedUp(open, inside, 0.0F), close, 0.0F);
    }

    // A part of a sum, with brackets put back where what it is made of binds more loosely than the
    // sign it stands beside.
    [[nodiscard]] Drawing partOf(std::size_t at, std::size_t whole, float size, int deep,
                                 bool onTheRight) {
        Drawing inside = of(at, size, deep + 1);
        const int mine = bindingOf(*m_equation, at);
        const int theirs = bindingOf(*m_equation, whole);
        const Operation operation = m_equation->steps[whole].operation;
        const bool takesFrom = operation == Operation::Subtract || operation == Operation::Divide;
        if (mine < theirs || (onTheRight && mine == theirs && takesFrom)) {
            return bracketed(inside, size);
        }
        return inside;
    }

    [[nodiscard]] Drawing beside(const Equation::Step& step, std::size_t at, float size, int deep) {
        const Drawing left = partOf(step.left, at, size, deep, false);
        const Drawing right = partOf(step.right, at, size, deep, true);
        const Drawing sign = wordOf(std::string{signOf(step.operation)}, size);
        const float gap = pageUnitsOfPoints(size) * kSignRoom;
        return joinedUp(joinedUp(left, sign, gap), right, gap);
    }

    [[nodiscard]] Drawing withSign(const Equation::Step& step, float size, int deep) {
        const Drawing sign = wordOf(std::string{kMinus}, size);
        Drawing inside = of(step.left, size, deep + 1);
        if (bindingOf(*m_equation, step.left) <= kBindsLoosest) {
            inside = bracketed(inside, size);
        }
        return joinedUp(sign, inside, 0.0F);
    }

    // One part over the other with a bar between, which is what division is written as everywhere
    // but on a keyboard.
    [[nodiscard]] Drawing fraction(const Equation::Step& step, float size, int deep) {
        const Drawing over = of(step.left, size, deep + 1);
        const Drawing under = of(step.right, size, deep + 1);
        const float room = pageUnitsOfPoints(size) * kFractionRoom;
        const float thick = std::max(pageUnitsOfPoints(size) * kBarThickness, 1.0F);
        const float wide = std::max(over.width, under.width) + (2.0F * room);
        const float bar = over.height + room;

        Drawing put = movedBy(over, (wide - over.width) * kHalfway, 0.0F);
        const Drawing below = movedBy(under, (wide - under.width) * kHalfway, bar + thick + room);
        put.glyphs.insert(put.glyphs.end(), below.glyphs.begin(), below.glyphs.end());
        put.bars.insert(put.bars.end(), below.bars.begin(), below.bars.end());
        put.bars.push_back(Bar{
            .area =
                Rect{
                    .left = 0.0F,
                    .top = bar,
                    .right = wide,
                    .bottom = bar + thick,
                },
        });
        put.width = wide;
        put.height = bar + thick + room + under.height;
        put.middle = bar + (thick * kHalfway);
        return put;
    }

    // What is raised is set smaller and lifted, so that its foot sits at the middle of what it is
    // raised over.
    [[nodiscard]] Drawing power(const Equation::Step& step, float size, int deep) {
        Drawing base = of(step.left, size, deep + 1);
        // Anything but a plain number is bracketed before it is raised, because a fraction or a
        // sum with a power over it says nothing without them.
        if (m_equation->steps[step.left].kind == Equation::Kind::Operation) {
            base = bracketed(base, size);
        }
        const float smaller = std::max(size * kRaisedShare, kSmallestType);
        const Drawing raised = of(step.right, smaller, deep + 1);
        // What is raised sits near the top of what it is raised over rather than at its middle,
        // so that a power over something tall does not slide down beside it.
        const float shoulder = pageUnitsOfPoints(size) * kLineRoom * kShoulderShare;
        const float carried = std::max(0.0F, raised.height - shoulder);
        const float top = std::max(0.0F, shoulder - raised.height);

        Drawing put = movedBy(base, 0.0F, carried);
        const Drawing above = movedBy(raised, base.width, top);
        put.glyphs.insert(put.glyphs.end(), above.glyphs.begin(), above.glyphs.end());
        put.bars.insert(put.bars.end(), above.bars.begin(), above.bars.end());
        put.width = base.width + raised.width;
        put.middle = base.middle + carried;
        put.height = base.height + carried;
        return put;
    }

    // The root sign with a roof drawn over everything it covers.
    [[nodiscard]] Drawing root(const Equation::Step& step, float size, int deep) {
        const Drawing inside = of(step.left, size, deep + 1);
        const float roof = std::max(pageUnitsOfPoints(size) * kBarThickness, 1.0F);
        const float room = pageUnitsOfPoints(size) * kRoofRoom;
        const float wide = pageUnitsOfPoints(size) * kRootWidth;

        Drawing put;
        put.glyphs.push_back(Glyph{
            .text = std::string{kRootSign},
            .at = Point{.x = 0.0F, .y = roof + room},
            .size = size,
        });
        const Drawing under = movedBy(inside, wide, roof + room);
        put.glyphs.insert(put.glyphs.end(), under.glyphs.begin(), under.glyphs.end());
        put.bars.insert(put.bars.end(), under.bars.begin(), under.bars.end());
        put.width = wide + inside.width;
        put.bars.push_back(Bar{
            .area =
                Rect{
                    .left = wide,
                    .top = 0.0F,
                    .right = put.width,
                    .bottom = roof,
                },
        });
        put.height = roof + room + inside.height;
        put.middle = roof + room + inside.middle;
        return put;
    }

    const Equation* m_equation;
    const Measure* m_measure;
};

}

Drawing movedBy(Drawing drawing, float dx, float dy) {
    for (Glyph& glyph : drawing.glyphs) {
        glyph.at.x += dx;
        glyph.at.y += dy;
    }
    for (Bar& bar : drawing.bars) {
        bar.area.left += dx;
        bar.area.right += dx;
        bar.area.top += dy;
        bar.area.bottom += dy;
    }
    return drawing;
}

Drawing laidOut(const Equation& equation, float size, const Measure& measure) {
    if (isEmpty(equation) || !measure) {
        return {};
    }
    Draughtsman draughtsman{equation, measure};
    return draughtsman.of(equation.whole, std::max(size, kSmallestType), 0);
}

}
