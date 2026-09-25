#pragma once

#include "core/Error.hpp"

#include <functional>
#include <optional>
#include <string>

namespace phvikapen::platform::update {

struct UpdateInfo {
    std::string version;
};

using HowFarAlong = std::function<void(int)>;

class IUpdater {
public:
    virtual ~IUpdater() = default;

    [[nodiscard]] virtual core::Result<std::optional<UpdateInfo>> checkForUpdates() = 0;

    [[nodiscard]] virtual core::Result<void> download(const UpdateInfo& update,
                                                      const HowFarAlong& told) = 0;

    [[nodiscard]] virtual core::Result<void> applyAndRestart(const UpdateInfo& update) = 0;
};

}
