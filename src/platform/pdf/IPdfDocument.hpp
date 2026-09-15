#pragma once

#include "core/Error.hpp"

namespace phvikapen::platform::pdf {

/// Size of a PDF page in points (1/72 inch).
struct PageSize {
    float width{};
    float height{};

    friend constexpr bool operator==(const PageSize&, const PageSize&) = default;
};

/// Read-only access to an opened PDF document.
///
/// TODO(M4): Implement with PDFium (prebuilt binaries downloaded by CMake with a pinned hash)
/// and add asynchronous page rendering into image buffers for GPU texture upload.
class IPdfDocument {
public:
    virtual ~IPdfDocument() = default;

    [[nodiscard]] virtual int pageCount() const noexcept = 0;

    /// Returns the size of the page at zero-based @p pageIndex.
    [[nodiscard]] virtual core::Result<PageSize> pageSize(int pageIndex) const = 0;
};

} // namespace phvikapen::platform::pdf
