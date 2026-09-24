#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "platform/text/IHandwriting.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace phvikapen::platform::text {
namespace {

TEST(HandwritingTest, SaysWhetherThisMachineReadsHandwriting) {
    const std::unique_ptr<IHandwriting> reader = openHandwriting();

#ifdef _WIN32
    ASSERT_NE(reader, nullptr) << "Windows reads handwriting";
    // The languages depend on what the machine has installed, and there may be none.
    const std::vector<std::string> languages = reader->languages();
    EXPECT_TRUE(languages.empty() || !languages.front().empty());
#else
    EXPECT_EQ(reader, nullptr) << "only Windows reads handwriting";
#endif
}

#ifdef _WIN32
TEST(HandwritingTest, ReadsNothingOutOfNothing) {
    const std::unique_ptr<IHandwriting> reader = openHandwriting();
    ASSERT_NE(reader, nullptr);

    const core::Result<std::vector<core::InkWord>> words = reader->read({});

    ASSERT_TRUE(words.has_value()) << words.error().message;
    EXPECT_TRUE(words->empty());
}

TEST(HandwritingTest, ReadsAStrokeWithoutFalling) {
    const std::unique_ptr<IHandwriting> reader = openHandwriting();
    ASSERT_NE(reader, nullptr);
    core::Stroke stroke{core::Uuid{}};
    for (int step = 0; step < 20; ++step) {
        const auto along = static_cast<float>(step);
        stroke.append(core::InkSample{.x = 100.0F + along, .y = 100.0F + (along * along * 0.1F)});
    }
    const std::vector<core::Stroke> strokes{stroke};

    const core::Result<std::vector<core::InkWord>> words = reader->read(strokes);

    // What the reader makes of one stroke is its own business; it must not fail over it.
    ASSERT_TRUE(words.has_value()) << words.error().message;
}
#endif

}
}
