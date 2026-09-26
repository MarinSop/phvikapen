#include "core/model/Recording.hpp"

#include "core/id/Uuid.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <utility>

namespace phvikapen::core {

Recording normalized(Recording recording) {
    recording.length = std::max<std::int64_t>(0, recording.length);
    recording.madeAt = std::max<std::int64_t>(0, recording.madeAt);
    if (recording.name.size() > Recording::kLongestName) {
        recording.name.resize(Recording::kLongestName);
    }
    return recording;
}

std::string plainRecordingName(std::int64_t madeAt) {
    const auto when = std::chrono::system_clock::time_point{std::chrono::milliseconds{madeAt}};
    return std::format("{:%Y-%m-%d %H:%M}", std::chrono::floor<std::chrono::minutes>(when));
}

std::size_t sayingAt(std::span<const Saying> sayings, std::int64_t at) noexcept {
    for (std::size_t step = 0; step < sayings.size(); ++step) {
        if (at >= sayings[step].from && at < sayings[step].to) {
            return step;
        }
    }
    return kNoSaying;
}

const Mark* markAt(std::span<const Mark> marks, const Uuid& recording, std::int64_t at) noexcept {
    const Mark* nearest = nullptr;
    for (const Mark& mark : marks) {
        if (mark.recording != recording || mark.at > at) {
            continue;
        }
        if (nearest == nullptr || mark.at > nearest->at) {
            nearest = &mark;
        }
    }
    return nearest;
}

const Mark* markOfThing(std::span<const Mark> marks, const Uuid& thing) noexcept {
    const auto found = std::ranges::find(marks, thing, &Mark::thing);
    return found == marks.end() ? nullptr : &*found;
}

}
