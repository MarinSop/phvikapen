#pragma once

#include "core/Error.hpp"
#include "core/model/Recording.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::platform::speech {

// What was made out of a recording, run by run, or why nothing could be.
using Heard = std::function<void(core::Result<std::vector<core::Saying>>)>;

// Reading a recording back as words. The same shape as reading handwriting: a name for the job,
// a reader per machine behind it, and a plain refusal where the machine carries none.
class ISpeech {
public:
    virtual ~ISpeech() = default;

    ISpeech() = default;
    ISpeech(const ISpeech&) = delete;
    ISpeech& operator=(const ISpeech&) = delete;
    ISpeech(ISpeech&&) = delete;
    ISpeech& operator=(ISpeech&&) = delete;

    // Reads a recording. The answer comes back through `heard`, on the thread this was asked on,
    // because reading takes long enough that nothing may wait for it.
    virtual void read(std::span<const std::byte> sound, const std::string& language,
                      Heard heard) = 0;

    // Gives up whatever is being read. Nothing comes back for it afterwards.
    virtual void giveUp() = 0;

    // The languages this machine can read speech in.
    [[nodiscard]] virtual std::vector<std::string> languages() = 0;
};

// A reader for this machine, or nothing where speech cannot be read.
[[nodiscard]] std::unique_ptr<ISpeech> openSpeech();

}
