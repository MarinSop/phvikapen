#include "platform/update/VelopackUpdater.hpp"

#include "core/Error.hpp"
#include "platform/update/Feed.hpp"
#include "platform/update/IUpdater.hpp"

#include <Velopack.hpp>

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace phvikapen::platform::update {
namespace {

[[nodiscard]] core::Error asError(const std::exception& failure) {
    return core::Error{.code = core::ErrorCode::IoFailure, .message = failure.what()};
}

[[nodiscard]] core::Error unknownFailure() {
    return core::Error{
        .code = core::ErrorCode::Unknown,
        .message = "the update service could not be reached",
    };
}

}

struct VelopackUpdater::Session {
    std::unique_ptr<Velopack::UpdateManager> manager;
    std::optional<Velopack::UpdateInfo> available;
};

VelopackUpdater::VelopackUpdater() noexcept = default;

VelopackUpdater::VelopackUpdater(std::unique_ptr<Session> session) noexcept
    : m_session{std::move(session)} {}

VelopackUpdater::~VelopackUpdater() = default;

core::Result<std::unique_ptr<VelopackUpdater>> VelopackUpdater::open(std::string_view feed,
                                                                     bool testVersions) {
    const std::string where{feed.empty() ? kUpdateFeed : feed};
    try {
        auto manager = where.starts_with("http")
                           ? std::make_unique<Velopack::UpdateManager>(
                                 std::make_unique<Velopack::GithubSource>(where, "", testVersions))
                           : std::make_unique<Velopack::UpdateManager>(
                                 std::make_unique<Velopack::FileSource>(where));
        auto session = std::make_unique<Session>(std::move(manager), std::nullopt);
        return std::make_unique<VelopackUpdater>(std::move(session));
    } catch (const std::exception& failure) {
        return std::unexpected{asError(failure)};
    } catch (...) {
        return std::unexpected{unknownFailure()};
    }
}

core::Result<std::optional<UpdateInfo>> VelopackUpdater::checkForUpdates() {
    if (!m_session) {
        return core::makeError(core::ErrorCode::Unsupported,
                               "no place to look for updates was given");
    }
    try {
        std::optional<Velopack::UpdateInfo> found = m_session->manager->CheckForUpdates();
        if (!found) {
            m_session->available.reset();
            return std::nullopt;
        }
        const std::string version = found->TargetFullRelease.Version;
        m_session->available = std::move(found);
        return UpdateInfo{.version = version};
    } catch (const std::exception& failure) {
        return std::unexpected{asError(failure)};
    } catch (...) {
        return std::unexpected{unknownFailure()};
    }
}

core::Result<void> VelopackUpdater::downloadAndRestart(const UpdateInfo& update) {
    if (!m_session || !m_session->available
        || m_session->available->TargetFullRelease.Version != update.version) {
        return core::makeError(core::ErrorCode::NotFound,
                               "the update to install was not the one that was found");
    }
    try {
        m_session->manager->DownloadUpdates(*m_session->available);
        m_session->manager->WaitExitThenApplyUpdates(*m_session->available);
        return {};
    } catch (const std::exception& failure) {
        return std::unexpected{asError(failure)};
    } catch (...) {
        return std::unexpected{unknownFailure()};
    }
}

}
