#include "core/storage/StrokeCodec.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr unsigned kVarintPayloadBits = 7;
constexpr std::uint64_t kVarintPayloadMask = 0x7F;
constexpr std::uint64_t kVarintContinuation = 0x80;
constexpr unsigned kMaxVarintBytes = 10;

[[nodiscard]] std::uint64_t zigzag(std::int64_t value) noexcept {
    const auto unsignedValue = static_cast<std::uint64_t>(value);
    const std::uint64_t signMask = value < 0 ? ~std::uint64_t{0} : std::uint64_t{0};
    return (unsignedValue << 1U) ^ signMask;
}

[[nodiscard]] std::int64_t unzigzag(std::uint64_t value) noexcept {
    return static_cast<std::int64_t>((value >> 1U) ^ (~(value & 1U) + 1U));
}

void appendVarint(std::vector<std::byte>& out, std::uint64_t value) {
    while (value >= kVarintContinuation) {
        out.push_back(static_cast<std::byte>((value & kVarintPayloadMask) | kVarintContinuation));
        value >>= kVarintPayloadBits;
    }
    out.push_back(static_cast<std::byte>(value));
}

class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes) noexcept : m_bytes{bytes} {}

    [[nodiscard]] bool failed() const noexcept { return m_failed; }

    [[nodiscard]] std::uint8_t readByte() noexcept {
        if (m_offset >= m_bytes.size()) {
            m_failed = true;
            return 0;
        }
        const auto value = static_cast<std::uint8_t>(m_bytes[m_offset]);
        ++m_offset;
        return value;
    }

    [[nodiscard]] std::uint64_t readVarint() noexcept {
        std::uint64_t value = 0;
        for (unsigned index = 0; index < kMaxVarintBytes; ++index) {
            const std::uint8_t byte = readByte();
            if (m_failed) {
                return 0;
            }
            value |= static_cast<std::uint64_t>(byte & kVarintPayloadMask)
                     << (index * kVarintPayloadBits);
            if ((byte & kVarintContinuation) == 0) {
                return value;
            }
        }
        m_failed = true;
        return 0;
    }

    [[nodiscard]] std::int64_t readSignedVarint() noexcept { return unzigzag(readVarint()); }

    [[nodiscard]] bool atEnd() const noexcept { return m_offset == m_bytes.size(); }

private:
    std::span<const std::byte> m_bytes;
    std::size_t m_offset{0};
    bool m_failed{false};
};

[[nodiscard]] std::int64_t quantize(float value, float step) noexcept {
    return std::llround(static_cast<double>(value) * static_cast<double>(step));
}

[[nodiscard]] float dequantize(std::int64_t value, float step) noexcept {
    return static_cast<float>(static_cast<double>(value) / static_cast<double>(step));
}

struct QuantizedSample {
    std::int64_t x{};
    std::int64_t y{};
    std::int64_t pressure{};
    std::int64_t tiltX{};
    std::int64_t tiltY{};
    std::int64_t timestamp{};
};

[[nodiscard]] QuantizedSample quantizeSample(const InkSample& sample) noexcept {
    return QuantizedSample{
        .x = quantize(sample.x, kStoredPositionsPerUnit),
        .y = quantize(sample.y, kStoredPositionsPerUnit),
        .pressure = quantize(sample.pressure, kStoredPressureSteps),
        .tiltX = quantize(sample.tiltX, kStoredTiltStepsPerDegree),
        .tiltY = quantize(sample.tiltY, kStoredTiltStepsPerDegree),
        .timestamp = sample.timestamp.count(),
    };
}

[[nodiscard]] InkSample dequantizeSample(const QuantizedSample& quantized) noexcept {
    return InkSample{
        .x = dequantize(quantized.x, kStoredPositionsPerUnit),
        .y = dequantize(quantized.y, kStoredPositionsPerUnit),
        .pressure = dequantize(quantized.pressure, kStoredPressureSteps),
        .tiltX = dequantize(quantized.tiltX, kStoredTiltStepsPerDegree),
        .tiltY = dequantize(quantized.tiltY, kStoredTiltStepsPerDegree),
        .timestamp = std::chrono::microseconds{quantized.timestamp},
    };
}

void appendSampleDifference(std::vector<std::byte>& out, const QuantizedSample& sample,
                            const QuantizedSample& previous) {
    appendVarint(out, zigzag(sample.x - previous.x));
    appendVarint(out, zigzag(sample.y - previous.y));
    appendVarint(out, zigzag(sample.pressure - previous.pressure));
    appendVarint(out, zigzag(sample.tiltX - previous.tiltX));
    appendVarint(out, zigzag(sample.tiltY - previous.tiltY));
    appendVarint(out, zigzag(sample.timestamp - previous.timestamp));
}

[[nodiscard]] QuantizedSample readSampleDifference(Reader& reader,
                                                   const QuantizedSample& previous) noexcept {
    QuantizedSample sample;
    sample.x = previous.x + reader.readSignedVarint();
    sample.y = previous.y + reader.readSignedVarint();
    sample.pressure = previous.pressure + reader.readSignedVarint();
    sample.tiltX = previous.tiltX + reader.readSignedVarint();
    sample.tiltY = previous.tiltY + reader.readSignedVarint();
    sample.timestamp = previous.timestamp + reader.readSignedVarint();
    return sample;
}

}

std::vector<std::byte> encodeStroke(const Stroke& stroke) {
    std::vector<std::byte> out;

    out.push_back(static_cast<std::byte>(kStrokeFormatVersion));
    for (const std::uint8_t byte : stroke.id().bytes()) {
        out.push_back(static_cast<std::byte>(byte));
    }

    const StrokeStyle& style = stroke.style();
    out.push_back(static_cast<std::byte>(style.color.red));
    out.push_back(static_cast<std::byte>(style.color.green));
    out.push_back(static_cast<std::byte>(style.color.blue));
    out.push_back(static_cast<std::byte>(style.color.alpha));
    appendVarint(out, zigzag(quantize(style.width, kStoredPositionsPerUnit)));

    const std::span<const InkSample> samples = stroke.samples();
    appendVarint(out, static_cast<std::uint64_t>(samples.size()));

    QuantizedSample previous;
    for (const InkSample& sample : samples) {
        const QuantizedSample quantized = quantizeSample(sample);
        appendSampleDifference(out, quantized, previous);
        previous = quantized;
    }
    return out;
}

Result<Stroke> decodeStroke(std::span<const std::byte> bytes) {
    Reader reader{bytes};

    const std::uint8_t version = reader.readByte();
    if (reader.failed()) {
        return makeError(ErrorCode::InvalidArgument, "stroke data is empty");
    }
    if (version > kStrokeFormatVersion) {
        return makeError(ErrorCode::Unsupported, "stroke was written by a newer format version");
    }

    Uuid::Bytes idBytes{};
    for (std::uint8_t& byte : idBytes) {
        byte = reader.readByte();
    }

    StrokeStyle style;
    style.color.red = reader.readByte();
    style.color.green = reader.readByte();
    style.color.blue = reader.readByte();
    style.color.alpha = reader.readByte();
    style.width = dequantize(reader.readSignedVarint(), kStoredPositionsPerUnit);

    const std::uint64_t sampleCount = reader.readVarint();
    if (reader.failed()) {
        return makeError(ErrorCode::InvalidArgument, "stroke data ends inside its header");
    }

    Stroke stroke{Uuid{idBytes}, style};
    QuantizedSample previous;
    for (std::uint64_t index = 0; index < sampleCount; ++index) {
        const QuantizedSample quantized = readSampleDifference(reader, previous);
        if (reader.failed()) {
            return makeError(ErrorCode::InvalidArgument, "stroke data ends inside its samples");
        }
        stroke.append(dequantizeSample(quantized));
        previous = quantized;
    }

    if (!reader.atEnd()) {
        return makeError(ErrorCode::InvalidArgument, "stroke data has trailing bytes");
    }
    return stroke;
}

}
