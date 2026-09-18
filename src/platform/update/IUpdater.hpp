#pragma once

#include "core/Error.hpp"

#include <optional>
#include <string>

namespace phvikapen::platform::update {

struct UpdateInfo {
    std::string version;
};

class IUpdater {
public:
    virtual ~IUpdater() = default;

    [[nodiscard]] virtual core::Result<std::optional<UpdateInfo>> checkForUpdates() = 0;

    [[nodiscard]] virtual core::Result<void> downloadAndRestart(const UpdateInfo& update) = 0;
};

}
