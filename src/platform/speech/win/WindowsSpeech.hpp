#pragma once

#include "platform/speech/ISpeech.hpp"

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>

#include <atomic>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

class QBuffer;
class QAudioDecoder;
class QTimer;

namespace phvikapen::platform::speech {

// Reading a recording back as words, by the reader Windows carries. The recording is kept as the
// same compressed sound everywhere, so it is first turned back into plain samples and only then
// handed to the reader, which reads nothing else.
class WindowsSpeech final : public ISpeech {
public:
    WindowsSpeech();
    ~WindowsSpeech() override;

    WindowsSpeech(const WindowsSpeech&) = delete;
    WindowsSpeech& operator=(const WindowsSpeech&) = delete;
    WindowsSpeech(WindowsSpeech&&) = delete;
    WindowsSpeech& operator=(WindowsSpeech&&) = delete;

    void read(std::span<const std::byte> sound, const std::string& language, Heard heard) override;

    void giveUp() override;

    [[nodiscard]] std::vector<std::string> languages() override;

private:
    using Answer = std::shared_ptr<Heard>;

    void takeSamples();
    void samplesDone();
    void nothingCameOfIt();
    void listen();
    void stopDecoding();

    // The answer is handed back through this one, so that it arrives on the thread that asked and
    // dies with the reader.
    QObject m_home;
    std::unique_ptr<QBuffer> m_held;
    std::unique_ptr<QAudioDecoder> m_decoder;
    // A recording that is not sound at all can leave the decoder saying nothing, neither done nor
    // in trouble, so a reading that stops making progress is given up rather than left waiting.
    std::unique_ptr<QTimer> m_waiting;
    QByteArray m_sound;
    QByteArray m_samples;
    QAudioFormat m_format;
    std::string m_language;
    Answer m_answer;
    std::jthread m_reading;
    std::shared_ptr<std::atomic_bool> m_wanted;
    bool m_apartment{};
};

}
