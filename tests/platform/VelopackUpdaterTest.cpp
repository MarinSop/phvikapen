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

TEST(VelopackUpdaterTest, RefusesToGetAnUpdateItNeverFound) {
    VelopackUpdater updater;
    int toldHowFar = -1;

    const core::Result<void> got = updater.download(
        UpdateInfo{.version = "9.9.9"}, [&toldHowFar](int howFar) { toldHowFar = howFar; });

    ASSERT_FALSE(got.has_value());
    EXPECT_EQ(got.error().code, core::ErrorCode::NotFound);
    EXPECT_EQ(toldHowFar, -1);
}

TEST(VelopackUpdaterTest, RefusesToInstallAnUpdateItNeverGot) {
    VelopackUpdater updater;

    const core::Result<void> installed = updater.applyAndRestart(UpdateInfo{.version = "9.9.9"});

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
