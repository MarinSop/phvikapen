#include "core/model/Picture.hpp"

#include <algorithm>
#include <cmath>

namespace phvikapen::core {

Picture normalized(Picture picture) noexcept {
    if (!std::isfinite(picture.at.x) || !std::isfinite(picture.at.y)) {
        picture.at = Point{};
    }
    if (!std::isfinite(picture.width)) {
        picture.width = Picture::kDefaultWidth;
    }
    if (!std::isfinite(picture.height)) {
        picture.height = Picture::kDefaultWidth;
    }
    picture.width = std::max(picture.width, Picture::kSmallest);
    picture.height = std::max(picture.height, Picture::kSmallest);
    if (!std::isfinite(picture.turn)) {
        picture.turn = 0.0F;
    }
    picture.turn = std::fmod(picture.turn, Picture::kFullTurn);
    return picture;
}

}
