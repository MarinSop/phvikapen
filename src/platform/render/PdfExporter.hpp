#pragma once

#include "core/Error.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>

namespace phvikapen::platform::render {

struct ExportProgress {
    int page{};
    int pageCount{};
};

using ProgressHandler = std::function<void(ExportProgress)>;

enum class ExportScope : std::uint8_t {
    // The sheet grows to hold ink written beside it.
    Everything,
    // Only the sheet, in the size the page was set to.
    Sheet,
    // Only the pages that came from an imported document.
    Document,
};

struct ExportOptions {
    ExportScope scope{ExportScope::Everything};

    friend constexpr bool operator==(const ExportOptions&, const ExportOptions&) = default;
};

[[nodiscard]] core::Result<int>
exportNotebookToPdf(const std::filesystem::path& notebook, const std::filesystem::path& target,
                    ExportOptions options = ExportOptions(),
                    const ProgressHandler& onProgress = ProgressHandler());

}
