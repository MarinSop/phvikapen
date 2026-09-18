#include "platform/update/VelopackUpdater.hpp"

#include "core/Error.hpp"
#include "platform/update/IUpdater.hpp"

#include <gtest/gtest.h>

#include <QTemporaryDir>

#include <memory>
#include <optional>
#include <string>

namespace phvikapen::platform::update {
namespace {

TEST(VelopackUpdaterTest, SaysSoWhenTheApplicationIsNotInstalled) {
    const QTemporaryDir directory;

    const core::Result<std::unique_ptr<VelopackUpdater>> updater =
        VelopackUpdater::open(directory.path().toStdString());

    ASSERT_FALSE(updater.has_value());
    EXPECT_FALSE(updater.error().message.empty());
}

TEST(VelopackUpdaterTest, RefusesToInstallAnUpdateItNeverFound) {
    VelopackUpdater updater;

    const core::Result<void> installed = updater.downloadAndRestart(UpdateInfo{.version = "9.9.9"});

    ASSERT_FALSE(installed.has_value());
    EXPECT_EQ(installed.error().code, core::ErrorCode::NotFound);
}

TEST(VelopackUpdaterTest, AnUpdaterWithoutAPlaceToLookFindsNothing) {
    VelopackUpdater updater;

    const core::Result<std::optional<UpdateInfo>> found = updater.checkForUpdates();

    ASSERT_FALSE(found.has_value());
    EXPECT_EQ(found.error().code, core::ErrorCode::Unsupported);
}

}
}
