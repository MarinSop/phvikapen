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

/// A stroke that looks like handwriting: a smooth arc sampled at 125 Hz.
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
        // Time is stored exactly, because replaying input depends on it.
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

    // The raw samples are 24 bytes each; differences of a smooth stroke are far smaller.
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

} // namespace
} // namespace phvikapen::core
