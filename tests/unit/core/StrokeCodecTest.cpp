#include "core/storage/StrokeCodec.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

using std::chrono::microseconds;

[[nodiscard]] Stroke makeHandwrittenStroke(int sampleCount) {
    Uuid7Generator ids;
    Stroke stroke{ids.next(),
                  StrokeStyle{.color = Color{.red = 20, .green = 30, .blue = 40}, .width = 2.5F}};
    for (int i = 0; i < sampleCount; ++i) {
        const auto step = static_cast<float>(i);
        stroke.append(InkSample{
            .x = 100.0F + (step * 1.7F),
            .y = 200.0F + (20.0F * std::sin(step / 10.0F)),
            .pressure = 0.4F + (0.2F * std::sin(step / 25.0F)),
            .tiltX = 15.5F,
            .tiltY = -7.25F,
            .timestamp = microseconds{8'000 * i},
        });
    }
    return stroke;
}

TEST(StrokeCodecTest, RoundTripKeepsIdentityStyleAndSampleCount) {
    const Stroke original = makeHandwrittenStroke(50);

    const Result<Stroke> decoded = decodeStroke(encodeStroke(original));

    ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
    EXPECT_EQ(decoded->id(), original.id());
    EXPECT_EQ(decoded->style().color, original.style().color);
    EXPECT_NEAR(decoded->style().width, original.style().width, 1.0F / kStoredPositionsPerUnit);
    EXPECT_EQ(decoded->samples().size(), original.samples().size());
}

TEST(StrokeCodecTest, SquareEndsSurviveTheRoundTrip) {
    Uuid7Generator ids;
    Stroke stroke{ids.next(), StrokeStyle{.width = 3.0F, .roundEnds = false}};
    stroke.append(InkSample{.x = 1.0F, .y = 2.0F});
    stroke.append(InkSample{.x = 9.0F, .y = 2.0F});

    const Result<Stroke> decoded = decodeStroke(encodeStroke(stroke));

    ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
    EXPECT_FALSE(decoded->style().roundEnds);
}

TEST(StrokeCodecTest, AStrokeWrittenBeforeSquareEndsStillHasRoundOnes) {
    const Stroke original = makeHandwrittenStroke(4);
    std::vector<std::byte> older = encodeStroke(original);
    ASSERT_FALSE(older.empty());
    // An older writer left no room for the ends and stopped a byte earlier.
    older.front() = static_cast<std::byte>(kSquareEndsVersion - 1);
    const auto endsAt = static_cast<std::size_t>(1 + Uuid::Bytes{}.size() + 4 + 1);
    older.erase(older.begin() + static_cast<std::ptrdiff_t>(endsAt));

    const Result<Stroke> decoded = decodeStroke(older);

    ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
    EXPECT_TRUE(decoded->style().roundEnds);
    EXPECT_EQ(decoded->samples().size(), original.samples().size());
}

TEST(StrokeCodecTest, RoundTripKeepsSamplesWithinTheStoredPrecision) {
    const Stroke original = makeHandwrittenStroke(200);

    const Result<Stroke> decoded = decodeStroke(encodeStroke(original));

    ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
    for (std::size_t i = 0; i < original.samples().size(); ++i) {
        const InkSample& before = original.samples()[i];
        const InkSample& after = decoded->samples()[i];
        EXPECT_NEAR(after.x, before.x, 1.0F / kStoredPositionsPerUnit);
        EXPECT_NEAR(after.y, before.y, 1.0F / kStoredPositionsPerUnit);
        EXPECT_NEAR(after.pressure, before.pressure, 1.0F / kStoredPressureSteps);
        EXPECT_NEAR(after.tiltX, before.tiltX, 1.0F / kStoredTiltStepsPerDegree);
        EXPECT_NEAR(after.tiltY, before.tiltY, 1.0F / kStoredTiltStepsPerDegree);
        EXPECT_EQ(after.timestamp, before.timestamp);
    }
}

TEST(StrokeCodecTest, EmptyStrokeSurvivesTheRoundTrip) {
    Uuid7Generator ids;
    const Stroke original{ids.next()};

    const Result<Stroke> decoded = decodeStroke(encodeStroke(original));

    ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
    EXPECT_TRUE(decoded->empty());
    EXPECT_EQ(decoded->id(), original.id());
}

TEST(StrokeCodecTest, HandwritingCostsOnlyAFewBytesPerSample) {
    const Stroke stroke = makeHandwrittenStroke(1'000);

    const std::vector<std::byte> encoded = encodeStroke(stroke);

    const std::size_t bytesPerSample = encoded.size() / stroke.samples().size();
    EXPECT_LT(bytesPerSample, 10U) << "encoded size " << encoded.size();
}

TEST(StrokeCodecTest, RejectsEmptyData) {
    const Result<Stroke> decoded = decodeStroke({});

    ASSERT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error().code, ErrorCode::InvalidArgument);
}

TEST(StrokeCodecTest, RejectsTruncatedData) {
    const std::vector<std::byte> encoded = encodeStroke(makeHandwrittenStroke(20));

    for (const std::size_t length : {std::size_t{1}, encoded.size() / 3, encoded.size() - 1}) {
        const Result<Stroke> decoded = decodeStroke(std::span{encoded}.first(length));
        EXPECT_FALSE(decoded.has_value()) << "length " << length;
    }
}

TEST(StrokeCodecTest, RejectsTrailingBytes) {
    std::vector<std::byte> encoded = encodeStroke(makeHandwrittenStroke(5));
    encoded.push_back(std::byte{0});

    const Result<Stroke> decoded = decodeStroke(encoded);

    ASSERT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error().code, ErrorCode::InvalidArgument);
}

TEST(StrokeCodecTest, RejectsANewerFormatVersion) {
    std::vector<std::byte> encoded = encodeStroke(makeHandwrittenStroke(5));
    encoded.front() = static_cast<std::byte>(kStrokeFormatVersion + 1);

    const Result<Stroke> decoded = decodeStroke(encoded);

    ASSERT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error().code, ErrorCode::Unsupported);
}

}
}
