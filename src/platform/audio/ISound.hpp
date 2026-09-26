#pragma once

#include "core/Error.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace phvikapen::platform::audio {

// What a recording and a playing tell whoever is listening. Everything comes back on the thread
// the sound was asked for on.
class ISoundSink {
public:
    virtual ~ISoundSink() = default;

    ISoundSink() = default;
    ISoundSink(const ISoundSink&) = delete;
    ISoundSink& operator=(const ISoundSink&) = delete;
    ISoundSink(ISoundSink&&) = delete;
    ISoundSink& operator=(ISoundSink&&) = delete;

    // How far into the recording the microphone has got, in milliseconds.
    virtual void recordingReached(std::int64_t at) = 0;

    // The recording is finished, and here is what was recorded.
    virtual void recordingMade(std::vector<std::byte> sound, std::int64_t length) = 0;

    virtual void recordingFailed(const core::Error& why) = 0;

    // How far into the playing the sound has got, and how long the whole of it runs.
    virtual void playingReached(std::int64_t at, std::int64_t length) = 0;

    // The playing stopped, either because it reached the end or because it was told to.
    virtual void playingStopped() = 0;

    virtual void playingFailed(const core::Error& why) = 0;
};

// Making a recording and playing one back. One of these keeps one recording and one playing at a
// time, which is what a reader can attend to.
class ISound {
public:
    virtual ~ISound() = default;

    ISound() = default;
    ISound(const ISound&) = delete;
    ISound& operator=(const ISound&) = delete;
    ISound(ISound&&) = delete;
    ISound& operator=(ISound&&) = delete;

    virtual void listenTo(ISoundSink* sink) = 0;

    // Whether this machine can listen at all: whether there is a microphone, and whether the
    // reader has allowed it to be used.
    [[nodiscard]] virtual bool canRecord() const = 0;

    // Asks the machine for leave to listen, where the machine asks. Whether it was given comes
    // back through `canRecord` once the reader has answered.
    virtual void askToRecord() = 0;

    [[nodiscard]] virtual core::Result<void> startRecording() = 0;
    virtual void pauseRecording() = 0;
    virtual void carryOnRecording() = 0;

    // Finishes the recording. What was recorded comes back through the sink.
    virtual void stopRecording() = 0;

    [[nodiscard]] virtual bool recording() const = 0;
    [[nodiscard]] virtual bool recordingPaused() const = 0;

    [[nodiscard]] virtual core::Result<void> play(std::span<const std::byte> sound) = 0;
    virtual void pausePlaying() = 0;
    virtual void carryOnPlaying() = 0;
    virtual void stopPlaying() = 0;
    virtual void goTo(std::int64_t at) = 0;

    [[nodiscard]] virtual bool playing() const = 0;
};

// Sound for this machine, or nothing where the toolkit could not open it.
[[nodiscard]] std::unique_ptr<ISound> openSound();

}
