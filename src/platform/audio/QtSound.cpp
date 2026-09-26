#include "platform/audio/QtSound.hpp"

#include "core/Error.hpp"
#include "platform/audio/ISound.hpp"

#include <QAudioDevice>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QPermission>
#include <QUrl>
#include <QtCore/QCoreApplication>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::platform::audio {
namespace {

constexpr int kQuality = static_cast<int>(QMediaRecorder::NormalQuality);

[[nodiscard]] std::vector<std::byte> bytesOf(const QByteArray& held) {
    std::vector<std::byte> bytes;
    bytes.reserve(static_cast<std::size_t>(held.size()));
    for (const char letter : held) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(letter)));
    }
    return bytes;
}

[[nodiscard]] QByteArray heldOf(std::span<const std::byte> sound) {
    QByteArray held;
    held.resize(static_cast<qsizetype>(sound.size()));
    for (std::size_t step = 0; step < sound.size(); ++step) {
        held[static_cast<qsizetype>(step)] = static_cast<char>(sound[step]);
    }
    return held;
}

}

QtSound::QtSound(QObject* parent) : QObject(parent) {
    m_session.setAudioInput(&m_microphone);
    m_session.setRecorder(&m_recorder);
    QMediaFormat format;
    format.setFileFormat(QMediaFormat::FileFormat::Mpeg4Audio);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    m_recorder.setMediaFormat(format);
    m_recorder.setQuality(static_cast<QMediaRecorder::Quality>(kQuality));

    connect(&m_recorder, &QMediaRecorder::durationChanged, this, [this](qint64 at) {
        m_reached = at;
        if (m_sink != nullptr) {
            m_sink->recordingReached(at);
        }
    });
    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
            [this](QMediaRecorder::RecorderState state) {
                if (state == QMediaRecorder::StoppedState && m_wanted) {
                    m_wanted = false;
                    takeWhatWasRecorded();
                }
            });
    connect(&m_recorder, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error error, const QString& said) {
                if (error == QMediaRecorder::NoError) {
                    return;
                }
                m_wanted = false;
                forgetTheFile();
                if (m_sink != nullptr) {
                    m_sink->recordingFailed(core::Error{
                        .code = core::ErrorCode::IoFailure,
                        .message =
                            said.isEmpty() ? "the recording could not be made" : said.toStdString(),
                    });
                }
            });

    connect(&m_player, &QMediaPlayer::positionChanged, this, [this](qint64 at) {
        if (m_sink != nullptr) {
            m_sink->playingReached(at, m_player.duration());
        }
    });
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
                if (state == QMediaPlayer::StoppedState && m_sink != nullptr) {
                    m_sink->playingStopped();
                }
            });
    connect(&m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error error, const QString& said) {
                if (error == QMediaPlayer::NoError || m_sink == nullptr) {
                    return;
                }
                m_sink->playingFailed(core::Error{
                    .code = core::ErrorCode::IoFailure,
                    .message =
                        said.isEmpty() ? "the recording could not be played" : said.toStdString(),
                });
            });
}

QtSound::~QtSound() {
    m_sink = nullptr;
    m_recorder.stop();
    m_player.stop();
}

void QtSound::listenTo(ISoundSink* sink) {
    m_sink = sink;
}

bool QtSound::canRecord() const {
    if (QMediaDevices::defaultAudioInput().isNull()) {
        return false;
    }
    const QMicrophonePermission leave;
    return qApp->checkPermission(leave) == Qt::PermissionStatus::Granted;
}

void QtSound::askToRecord() {
    const QMicrophonePermission leave;
    if (qApp->checkPermission(leave) != Qt::PermissionStatus::Undetermined) {
        return;
    }
    qApp->requestPermission(leave, this, [](const QPermission&) {});
}

core::Result<void> QtSound::startRecording() {
    if (recording()) {
        return {};
    }
    if (QMediaDevices::defaultAudioInput().isNull()) {
        return core::makeError(core::ErrorCode::Unsupported,
                               "this machine has nothing to listen with");
    }
    if (!canRecord()) {
        askToRecord();
        return core::makeError(core::ErrorCode::Unsupported,
                               "the application has not been allowed to listen");
    }

    auto writing = std::make_unique<QTemporaryFile>(QDir::tempPath()
                                                    + QStringLiteral("/phvikapen-XXXXXX.m4a"));
    writing->setAutoRemove(true);
    if (!writing->open()) {
        return core::makeError(core::ErrorCode::IoFailure,
                               "there is nowhere to write the recording");
    }
    const QString where = writing->fileName();
    writing->close();
    m_writing = std::move(writing);

    m_reached = 0;
    m_wanted = true;
    m_recorder.setOutputLocation(QUrl::fromLocalFile(where));
    m_recorder.record();
    return {};
}

void QtSound::pauseRecording() {
    if (m_recorder.recorderState() == QMediaRecorder::RecordingState) {
        m_recorder.pause();
    }
}

void QtSound::carryOnRecording() {
    if (m_recorder.recorderState() == QMediaRecorder::PausedState) {
        m_recorder.record();
    }
}

void QtSound::stopRecording() {
    if (m_recorder.recorderState() == QMediaRecorder::StoppedState) {
        return;
    }
    m_recorder.stop();
}

bool QtSound::recording() const {
    return m_recorder.recorderState() != QMediaRecorder::StoppedState;
}

bool QtSound::recordingPaused() const {
    return m_recorder.recorderState() == QMediaRecorder::PausedState;
}

void QtSound::takeWhatWasRecorded() {
    const std::int64_t length = m_reached;
    if (m_writing == nullptr) {
        return;
    }
    QFile written{m_writing->fileName()};
    if (!written.open(QIODevice::ReadOnly)) {
        forgetTheFile();
        if (m_sink != nullptr) {
            m_sink->recordingFailed(core::Error{
                .code = core::ErrorCode::IoFailure,
                .message = "the recording could not be read back",
            });
        }
        return;
    }
    const QByteArray held = written.readAll();
    written.close();
    forgetTheFile();
    if (held.isEmpty()) {
        if (m_sink != nullptr) {
            m_sink->recordingFailed(core::Error{
                .code = core::ErrorCode::IoFailure,
                .message = "nothing at all was recorded",
            });
        }
        return;
    }
    if (m_sink != nullptr) {
        m_sink->recordingMade(bytesOf(held), length);
    }
}

void QtSound::forgetTheFile() {
    m_writing = nullptr;
}

core::Result<void> QtSound::play(std::span<const std::byte> sound) {
    if (sound.empty()) {
        return core::makeError(core::ErrorCode::NotFound, "there is nothing here to play");
    }
    stopPlaying();
    m_sound = heldOf(sound);
    m_heard.close();
    m_heard.setBuffer(&m_sound);
    if (!m_heard.open(QIODevice::ReadOnly)) {
        return core::makeError(core::ErrorCode::IoFailure, "the recording could not be opened");
    }
    m_player.setSourceDevice(&m_heard, QUrl{QStringLiteral("phvikapen://recording.m4a")});
    m_player.play();
    return {};
}

void QtSound::pausePlaying() {
    if (m_player.playbackState() == QMediaPlayer::PlayingState) {
        m_player.pause();
    }
}

void QtSound::carryOnPlaying() {
    if (m_player.playbackState() == QMediaPlayer::PausedState) {
        m_player.play();
    }
}

void QtSound::stopPlaying() {
    m_player.stop();
    m_player.setSourceDevice(nullptr);
    m_heard.close();
}

void QtSound::goTo(std::int64_t at) {
    if (m_player.isSeekable()) {
        m_player.setPosition(at);
    }
}

bool QtSound::playing() const {
    return m_player.playbackState() == QMediaPlayer::PlayingState;
}

std::unique_ptr<ISound> openSound() {
    return std::make_unique<QtSound>();
}

}
