#pragma once

#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

namespace phvikapen::test {

class TemporaryNotebook {
public:
    TemporaryNotebook() {
        core::Uuid7Generator ids;
        m_path = std::filesystem::temp_directory_path()
                 / ("phvikapen-" + ids.next().toString() + ".phvika");
        m_walPath = m_path.string() + "-wal";
        m_shmPath = m_path.string() + "-shm";
    }

    ~TemporaryNotebook() {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
        std::filesystem::remove(m_walPath, ignored);
        std::filesystem::remove(m_shmPath, ignored);
    }

    TemporaryNotebook(const TemporaryNotebook&) = delete;
    TemporaryNotebook& operator=(const TemporaryNotebook&) = delete;
    TemporaryNotebook(TemporaryNotebook&&) = delete;
    TemporaryNotebook& operator=(TemporaryNotebook&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
    std::filesystem::path m_walPath;
    std::filesystem::path m_shmPath;
};

[[nodiscard]] inline core::Stroke makeStroke(core::Uuid7Generator& ids, float originX) {
    core::Stroke stroke{ids.next(), core::StrokeStyle{.width = 3.0F}};
    for (int i = 0; i < 20; ++i) {
        const auto step = static_cast<float>(i);
        stroke.append(core::InkSample{
            .x = originX + step,
            .y = 50.0F + step,
            .pressure = 0.7F,
            .timestamp = std::chrono::microseconds{8'000 * i},
        });
    }
    return stroke;
}

}
