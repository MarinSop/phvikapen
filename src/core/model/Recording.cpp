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
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] bool endsALine(const std::string& said) noexcept {
    if (said.empty()) {
        return false;
    }
    const char last = said.back();
    return last == '.' || last == '?' || last == '!';
}

[[nodiscard]] bool startsALine(const Saying& before, std::int64_t from) noexcept {
    return from - before.to > kPause || before.text.size() >= kLongestLine
           || endsALine(before.text);
}

}

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

std::vector<Saying> linesOf(std::span<const Saying> words) {
    std::vector<Saying> lines;
    for (const Saying& word : words) {
        if (word.text.empty()) {
            continue;
        }
        if (lines.empty() || startsALine(lines.back(), word.from)) {
            lines.push_back(Saying{
                .from = word.from,
                .to = std::max(word.from, word.to),
                .text = word.text,
            });
            continue;
        }
        Saying& line = lines.back();
        line.text += ' ';
        line.text += word.text;
        line.to = std::max(line.to, word.to);
    }
    return lines;
}

}
