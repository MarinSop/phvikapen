#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <gtest/gtest.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QTemporaryDir>

#include <filesystem>
#include <iostream>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr auto kNotebookVariable = "PHVIKAPEN_CRASH_NOTEBOOK";
constexpr int kStrokesBeforeTheKill = 5;
constexpr int kSamplesPerStroke = 40;

[[nodiscard]] Stroke scribble(Uuid7Generator& ids, int number) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 2.0F}};
    for (int i = 0; i < kSamplesPerStroke; ++i) {
        stroke.append(InkSample{
            .x = static_cast<float>(i),
            .y = static_cast<float>(number * 10),
        });
    }
    return stroke;
}

// Runs in a second process, which the test below kills in the middle of a stroke.
TEST(CrashSafetyTest, DISABLED_WritesUntilKilled) {
    const QString path = qEnvironmentVariable(kNotebookVariable);
    ASSERT_FALSE(path.isEmpty());

    Uuid7Generator ids;
    const std::filesystem::path file{path.toStdU16String()};
    Uuid page;
    {
        const Result<NotebookStore> store = NotebookStore::open(file);
        ASSERT_TRUE(store.has_value()) << store.error().message;
        const Result<NotebookOutline> outline = store->readOutline();
        ASSERT_TRUE(outline.has_value()) << outline.error().message;
        page = outline->sections.front().pages.front().id;
    }

    StorageThread storage{file, [](const Error&) {}};

    for (int number = 0;; ++number) {
        storage.insertStroke(page, PlacedStroke{
                                       .ordinal = number,
                                       .stroke = scribble(ids, number),
                                   });
        storage.waitUntilIdle();
        std::cout << "wrote " << (number + 1) << '\n' << std::flush;
    }
}

TEST(CrashSafetyTest, ANotebookSurvivesTheApplicationBeingKilled) {
    const QTemporaryDir directory;
    const QString path = directory.filePath("Killed.phvika");

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(kNotebookVariable, path);
    QProcess writer;
    writer.setProcessEnvironment(environment);
    writer.setProcessChannelMode(QProcess::MergedChannels);
    writer.start(QCoreApplication::applicationFilePath(),
                 {
                     "--gtest_also_run_disabled_tests",
                     "--gtest_filter=CrashSafetyTest.DISABLED_WritesUntilKilled",
                 });
    ASSERT_TRUE(writer.waitForStarted());

    QByteArray seen;
    const QByteArray wanted = "wrote " + QByteArray::number(kStrokesBeforeTheKill);
    while (!seen.contains(wanted)) {
        ASSERT_TRUE(writer.waitForReadyRead(30'000)) << seen.toStdString();
        seen.append(writer.readAll());
    }

    writer.kill();
    ASSERT_TRUE(writer.waitForFinished());

    const Result<NotebookStore> reopened =
        NotebookStore::open(std::filesystem::path{path.toStdU16String()});
    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    const Result<NotebookOutline> outline = reopened->readOutline();
    ASSERT_TRUE(outline.has_value()) << outline.error().message;

    int found = 0;
    for (const SectionInfo& section : outline->sections) {
        for (const PageInfo& info : section.pages) {
            const Result<std::vector<PlacedStroke>> strokes = reopened->strokesOfPage(info.id);
            ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
            found += static_cast<int>(strokes->size());
        }
    }
    EXPECT_GE(found, kStrokesBeforeTheKill);
    for (const SectionInfo& section : outline->sections) {
        for (const PageInfo& info : section.pages) {
            for (const PlacedStroke& placed : reopened->strokesOfPage(info.id).value()) {
                EXPECT_EQ(placed.stroke.samples().size(),
                          static_cast<std::size_t>(kSamplesPerStroke));
            }
        }
    }
}

}
}
