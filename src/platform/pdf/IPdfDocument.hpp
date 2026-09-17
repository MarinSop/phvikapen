#pragma once

#include "core/Error.hpp"

namespace phvikapen::platform::pdf {

struct PageSize {
    float width{};
    float height{};

    friend constexpr bool operator==(const PageSize&, const PageSize&) = default;
};

// TODO(M4): Implement with PDFium.
class IPdfDocument {
public:
    virtual ~IPdfDocument() = default;

    [[nodiscard]] virtual int pageCount() const noexcept = 0;

    [[nodiscard]] virtual core::Result<PageSize> pageSize(int pageIndex) const = 0;
};

}
