#pragma once

#include "core/Error.hpp"

#include <optional>
#include <string>

namespace phvikapen::platform::update {

/// A newer release that can be installed.
struct UpdateInfo {
    /// Semantic version of the release, for example "1.2.0".
    std::string version;
};

/// Checks for, downloads and applies application updates.
///
/// TODO(M5): Implement with the Velopack update manager against GitHub Releases.
class IUpdater {
public:
    virtual ~IUpdater() = default;

    /// Returns the newest available release, or std::nullopt when up to date.
    [[nodiscard]] virtual core::Result<std::optional<UpdateInfo>> checkForUpdates() = 0;

    /// Downloads @p update and restarts the application into it.
    [[nodiscard]] virtual core::Result<void> downloadAndRestart(const UpdateInfo& update) = 0;
};

} // namespace phvikapen::platform::update
