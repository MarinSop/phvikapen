#pragma once

#include "core/id/ContentId.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace phvikapen::core {

enum class AssetKind : std::uint8_t {
    Pdf,
    Image,
};

struct Asset {
    ContentId id;
    AssetKind kind{AssetKind::Pdf};
    std::string name;
    std::vector<std::byte> data;

    friend bool operator==(const Asset&, const Asset&) = default;
};

struct PageMedia {
    ContentId asset;
    int index{};

    friend bool operator==(const PageMedia&, const PageMedia&) = default;
};

}
