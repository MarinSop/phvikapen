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

[[nodiscard]] core::Result<int> exportNotebookToPdf(const std::filesystem::path& notebook,
                                                    const std::filesystem::path& target,
                                                    const ProgressHandler& onProgress = {});

}
