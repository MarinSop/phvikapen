#pragma once

#include "platform/audio/ISound.hpp"

#include <QAudioInput>
#include <QBuffer>
#include <QByteArray>
#include <QMediaCaptureSession>
#include <QMediaPlayer>
#include <QMediaRecorder>
#include <QObject>
#include <QTemporaryFile>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace phvikapen::platform::audio {

// Sound through the toolkit: a microphone written to a file while recording, and what was recorded
// played back out of memory.
class QtSound final : public QObject, public ISound {
    Q_OBJECT

public:
    explicit QtSound(QObject* parent = nullptr);
    ~QtSound() override;

    QtSound(const QtSound&) = delete;
    QtSound& operator=(const QtSound&) = delete;
    QtSound(QtSound&&) = delete;
    QtSound& operator=(QtSound&&) = delete;

    void listenTo(ISoundSink* sink) override;

    [[nodiscard]] bool canRecord() const override;
    void askToRecord() override;

    [[nodiscard]] core::Result<void> startRecording() override;
    void pauseRecording() override;
    void carryOnRecording() override;
    void stopRecording() override;
    [[nodiscard]] bool recording() const override;
    [[nodiscard]] bool recordingPaused() const override;

    [[nodiscard]] core::Result<void> play(std::span<const std::byte> sound) override;
    void pausePlaying() override;
    void carryOnPlaying() override;
    void stopPlaying() override;
    void goTo(std::int64_t at) override;
    [[nodiscard]] bool playing() const override;

private:
    void takeWhatWasRecorded();
    void forgetTheFile();

    ISoundSink* m_sink{nullptr};
    QAudioInput m_microphone;
    QMediaCaptureSession m_session;
    QMediaRecorder m_recorder;
    QMediaPlayer m_player;
    QBuffer m_heard;
    QByteArray m_sound;
    std::unique_ptr<QTemporaryFile> m_writing;
    std::int64_t m_reached{0};
    bool m_wanted{false};
};

}
