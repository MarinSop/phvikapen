#pragma once

#include "core/Error.hpp"

#include <cstdint>
#include <vector>

namespace phvikapen::platform::pdf {

struct PageSize {
    float width{};
    float height{};

    friend constexpr bool operator==(const PageSize&, const PageSize&) = default;
};

struct PageRegion {
    float left{};
    float top{};
    float width{};
    float height{};

    friend constexpr bool operator==(const PageRegion&, const PageRegion&) = default;
};

struct PageImage {
    int width{};
    int height{};
    std::vector<std::uint8_t> pixels;
};

class IPdfDocument {
public:
    IPdfDocument() = default;
    virtual ~IPdfDocument() = default;

    IPdfDocument(const IPdfDocument&) = delete;
    IPdfDocument& operator=(const IPdfDocument&) = delete;
    IPdfDocument(IPdfDocument&&) = delete;
    IPdfDocument& operator=(IPdfDocument&&) = delete;

    [[nodiscard]] virtual int pageCount() const noexcept = 0;

    [[nodiscard]] virtual core::Result<PageSize> pageSize(int pageIndex) const = 0;

    [[nodiscard]] virtual core::Result<PageImage> renderPage(int pageIndex, int widthInPixels,
                                                             int heightInPixels) const = 0;

    [[nodiscard]] virtual core::Result<PageImage> renderRegion(int pageIndex, int widthInPixels,
                                                               int heightInPixels,
                                                               const PageRegion& region) const = 0;
};

}
