#pragma once

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"

#include <string>
#include <vector>

namespace phvikapen::core {

// A word read from handwriting, where it sits on the page, and the strokes it was written with.
struct InkWord {
    std::string text;
    Rect box;
    std::vector<Uuid> strokes;

    friend bool operator==(const InkWord&, const InkWord&) = default;
};

struct FoundWord {
    Uuid pageId;
    InkWord word;

    friend bool operator==(const FoundWord&, const FoundWord&) = default;
};

}
