#pragma once

#include "core/Error.hpp"

#include <filesystem>
#include <functional>

namespace phvikapen::platform::render {

struct ExportProgress {
    int page{};
    int pageCount{};
};

using ProgressHandler = std::function<void(ExportProgress)>;

struct ExportOptions {
    // Pages grow to hold ink written beside the sheet; otherwise only the sheet is written.
    bool everything{true};

    friend constexpr bool operator==(const ExportOptions&, const ExportOptions&) = default;
};

[[nodiscard]] core::Result<int>
exportNotebookToPdf(const std::filesystem::path& notebook, const std::filesystem::path& target,
                    ExportOptions options = ExportOptions(),
                    const ProgressHandler& onProgress = ProgressHandler());

}
