#pragma once

#include "core/Error.hpp"
#include "platform/update/IUpdater.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace phvikapen::platform::update {

class VelopackUpdater final : public IUpdater {
public:
    struct Session;

    // An http address is a GitHub repository, anything else a folder of releases.
    [[nodiscard]] static core::Result<std::unique_ptr<VelopackUpdater>>
    open(std::string_view feed = {}, bool testVersions = false);

    VelopackUpdater() noexcept;

    explicit VelopackUpdater(std::unique_ptr<Session> session) noexcept;

    ~VelopackUpdater() override;

    VelopackUpdater(const VelopackUpdater&) = delete;
    VelopackUpdater& operator=(const VelopackUpdater&) = delete;
    VelopackUpdater(VelopackUpdater&&) = delete;
    VelopackUpdater& operator=(VelopackUpdater&&) = delete;

    [[nodiscard]] core::Result<std::optional<UpdateInfo>> checkForUpdates() override;

    [[nodiscard]] core::Result<void> downloadAndRestart(const UpdateInfo& update) override;

private:
    std::unique_ptr<Session> m_session;
};

}
