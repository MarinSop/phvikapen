#pragma once

#include "core/Error.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::platform::ocr {

// One run of words found in a picture, and where it sits: the corners are given as shares of the
// width and height of the picture, so that they mean the same whatever size it is shown at.
struct Found {
    std::string text;
    float left{};
    float top{};
    float right{};
    float bottom{};

    friend bool operator==(const Found&, const Found&) = default;
};

// What was made out of a picture, or why nothing could be.
using Read = std::function<void(core::Result<std::vector<Found>>)>;

// Reading the words in a picture. The same shape as reading handwriting and reading speech: a name
// for the job, a reader per machine behind it, and a plain refusal where the machine has none.
class IReadPicture {
public:
    virtual ~IReadPicture() = default;

    IReadPicture() = default;
    IReadPicture(const IReadPicture&) = delete;
    IReadPicture& operator=(const IReadPicture&) = delete;
    IReadPicture(IReadPicture&&) = delete;
    IReadPicture& operator=(IReadPicture&&) = delete;

    // Reads a picture, given as the bytes of the file it came in. The answer comes back through
    // `read`, on the thread this was asked on.
    virtual void read(std::span<const std::byte> picture, const std::string& language,
                      Read read) = 0;

    virtual void giveUp() = 0;

    // The languages this machine can read words in.
    [[nodiscard]] virtual std::vector<std::string> languages() = 0;
};

// A reader for this machine, or nothing where the words in a picture cannot be read.
[[nodiscard]] std::unique_ptr<IReadPicture> openReadPicture();

}
