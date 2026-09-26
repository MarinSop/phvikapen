#pragma once

#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::core {

// One run of what was said, with the moment it begins and the moment it ends.
struct Saying {
    std::int64_t from{};
    std::int64_t to{};
    std::string text;

    friend bool operator==(const Saying&, const Saying&) = default;
};

// How far the reading of a recording has got.
enum class Reading : std::uint8_t {
    // Nobody has asked for it to be read.
    Unasked,
    // It is being read now.
    Asked,
    // It has been read.
    Read,
    // The reading could not be done.
    Failed,
};

// What was said in a recording, as far as it has been read.
struct Said {
    Reading reading{Reading::Unasked};
    // Which language it was read in, empty where nobody has said.
    std::string language;
    // Why the reading could not be done, empty where it was.
    std::string trouble;
    std::vector<Saying> sayings;

    friend bool operator==(const Said&, const Said&) = default;
};

// A recording made while a page was being written on. What was said is kept once in the notebook,
// by what it contains, like every other thing a page carries that is made of bytes.
struct Recording {
    static constexpr std::size_t kLongestName = 120;

    Uuid id;
    ContentId sound;
    std::string name;
    // How long it runs, in milliseconds.
    std::int64_t length{};
    // When it was made, in milliseconds since the epoch.
    std::int64_t madeAt{};
    Said said;

    friend bool operator==(const Recording&, const Recording&) = default;
};

// A thing on the page and the moment in a recording it was put there. The thing is named by what
// it is rather than by where it sits or what it says, so that moving it, correcting what it says
// or restyling it all leave the mark pointing at the same thing.
struct Mark {
    Uuid recording;
    Uuid thing;
    // How far into the recording, in milliseconds.
    std::int64_t at{};

    friend bool operator==(const Mark&, const Mark&) = default;
};

[[nodiscard]] Recording normalized(Recording recording);

// The name a recording is given when nobody has named it, made from when it was recorded.
[[nodiscard]] std::string plainRecordingName(std::int64_t madeAt);

// Which saying is being heard at a moment, or nothing at all where none is.
[[nodiscard]] std::size_t sayingAt(std::span<const Saying> sayings, std::int64_t at) noexcept;

inline constexpr std::size_t kNoSaying = static_cast<std::size_t>(-1);

// What was marked at a moment or before it: the thing being written about when the recording
// reached that point.
[[nodiscard]] const Mark* markAt(std::span<const Mark> marks, const Uuid& recording,
                                 std::int64_t at) noexcept;

// Where in a recording a thing on the page was written, or nothing at all where it was not
// written while anything was being recorded.
[[nodiscard]] const Mark* markOfThing(std::span<const Mark> marks, const Uuid& thing) noexcept;

}
