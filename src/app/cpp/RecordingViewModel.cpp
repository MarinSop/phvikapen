#include "app/cpp/RecordingViewModel.hpp"

#include "app/cpp/NotebookViewModel.hpp"
#include "core/Error.hpp"
#include "platform/audio/ISound.hpp"

#include <QByteArray>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr qint64 kSecond = 1000;
constexpr qint64 kMinute = 60 * kSecond;
constexpr qint64 kHour = 60 * kMinute;
constexpr int kTens = 10;

[[nodiscard]] QString twoFigures(qint64 of) {
    return QStringLiteral("%1").arg(of, 2, kTens, QLatin1Char{'0'});
}

}

RecordingViewModel::RecordingViewModel(QObject* parent)
    : QObject(parent), m_sound{platform::audio::openSound()},
      m_speech{platform::speech::openSpeech()} {
    if (m_sound != nullptr) {
        m_sound->listenTo(this);
    }
}

RecordingViewModel::~RecordingViewModel() {
    if (m_sound != nullptr) {
        m_sound->listenTo(nullptr);
    }
    if (m_speech != nullptr) {
        m_speech->giveUp();
    }
}

QStringList RecordingViewModel::languages() const {
    QStringList said;
    if (m_speech == nullptr) {
        return said;
    }
    for (const std::string& language : m_speech->languages()) {
        said.append(QString::fromStdString(language));
    }
    return said;
}

void RecordingViewModel::readWhatWasSaid(const QString& recordingId, const QString& language) {
    if (m_notebook.isNull() || recordingId.isEmpty()) {
        return;
    }
    if (m_speech == nullptr) {
        const QString why = tr("This machine cannot read a recording back as words.");
        m_notebook->markReading(recordingId, static_cast<int>(core::Reading::Failed), why);
        emit cannotRead(why);
        return;
    }
    if (!m_readingNow.isEmpty()) {
        return;
    }
    m_readingNow = recordingId;
    m_readingLanguage = language;
    m_notebook->markReading(recordingId, static_cast<int>(core::Reading::Asked), QString{});
    emit readingChanged();
    m_notebook->wantSound(recordingId);
}

void RecordingViewModel::hearWhatWasSaid(const QString& recordingId, const QByteArray& sound) {
    if (m_speech == nullptr || m_notebook.isNull()) {
        return;
    }
    std::vector<std::byte> bytes;
    bytes.reserve(static_cast<std::size_t>(sound.size()));
    for (const char letter : sound) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(letter)));
    }
    const QString language = m_readingLanguage;
    m_speech->read(bytes, language.toStdString(),
                   [this, recordingId, language](core::Result<std::vector<core::Saying>> heard) {
                       if (recordingId != m_readingNow) {
                           return;
                       }
                       m_readingNow.clear();
                       emit readingChanged();
                       if (m_notebook.isNull()) {
                           return;
                       }
                       if (!heard) {
                           m_notebook->markReading(recordingId,
                                                   static_cast<int>(core::Reading::Failed),
                                                   QString::fromStdString(heard.error().message));
                           return;
                       }
                       QVariantList said;
                       said.reserve(static_cast<qsizetype>(heard->size()));
                       for (const core::Saying& saying : *heard) {
                           said.append(QVariantMap{
                               {QStringLiteral("from"), static_cast<qint64>(saying.from)},
                               {QStringLiteral("to"), static_cast<qint64>(saying.to)},
                               {QStringLiteral("text"), QString::fromStdString(saying.text)},
                           });
                       }
                       m_notebook->keepSayings(recordingId, said, language);
                   });
}

void RecordingViewModel::giveUpReading() {
    if (m_readingNow.isEmpty()) {
        return;
    }
    if (m_speech != nullptr) {
        m_speech->giveUp();
    }
    if (!m_notebook.isNull()) {
        m_notebook->markReading(m_readingNow, static_cast<int>(core::Reading::Unasked), QString{});
    }
    m_readingNow.clear();
    emit readingChanged();
}

NotebookViewModel* RecordingViewModel::notebook() const {
    return m_notebook.data();
}

void RecordingViewModel::setNotebook(NotebookViewModel* notebook) {
    if (m_notebook == notebook) {
        return;
    }
    if (m_recording) {
        stopRecording();
    }
    stopPlaying();
    if (!m_notebook.isNull()) {
        disconnect(m_notebook, nullptr, this, nullptr);
    }
    m_notebook = notebook;
    if (!m_notebook.isNull()) {
        connect(m_notebook, &NotebookViewModel::soundReady, this,
                &RecordingViewModel::soundArrived);
        connect(m_notebook, &NotebookViewModel::soundMissing, this,
                &RecordingViewModel::soundNotThere);
    }
    emit notebookChanged();
    emit workingChanged();
}

void RecordingViewModel::soundArrived(const QString& recordingId, const QByteArray& sound) {
    if (recordingId == m_readingNow) {
        hearWhatWasSaid(recordingId, sound);
        return;
    }
    if (recordingId != m_wantedId || m_sound == nullptr) {
        return;
    }
    std::vector<std::byte> bytes;
    bytes.reserve(static_cast<std::size_t>(sound.size()));
    for (const char letter : sound) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(letter)));
    }
    const core::Result<void> started = m_sound->play(bytes);
    if (!started) {
        takeTrouble(QString::fromStdString(started.error().message));
        return;
    }
    m_playingId = recordingId;
    m_playing = true;
    m_playedTo = 0;
    if (m_goingTo >= 0) {
        m_sound->goTo(m_goingTo);
        m_goingTo = -1;
    }
    emit playingChanged();
}

void RecordingViewModel::soundNotThere(const QString& recordingId) {
    if (recordingId == m_readingNow) {
        m_readingNow.clear();
        emit readingChanged();
        if (!m_notebook.isNull()) {
            m_notebook->markReading(recordingId, static_cast<int>(core::Reading::Failed),
                                    tr("This recording could not be found in the notebook."));
        }
        return;
    }
    if (recordingId != m_wantedId) {
        return;
    }
    m_wantedId.clear();
    m_goingTo = -1;
    takeTrouble(tr("This recording could not be found in the notebook."));
}

bool RecordingViewModel::canRecord() const {
    return m_sound != nullptr && m_sound->canRecord();
}

void RecordingViewModel::takeTrouble(const QString& why) {
    m_trouble = why;
    emit troubleChanged();
}

void RecordingViewModel::forgetTrouble() {
    if (m_trouble.isEmpty()) {
        return;
    }
    m_trouble.clear();
    emit troubleChanged();
}

void RecordingViewModel::tellTheNotebook() {
    if (m_notebook.isNull()) {
        return;
    }
    m_notebook->markFrom(m_recording ? m_recordingInto : QString{}, m_recordedFor);
}

void RecordingViewModel::startRecording() {
    if (m_sound == nullptr) {
        emit cannotRecord(tr("This machine has nothing to listen with."));
        return;
    }
    if (m_recording) {
        return;
    }
    if (m_notebook.isNull() || !m_notebook->loaded()) {
        emit cannotRecord(tr("Open a page before recording."));
        return;
    }
    forgetTrouble();
    const QString into = m_notebook->beginRecording();
    if (into.isEmpty()) {
        emit cannotRecord(tr("There is no page to record on."));
        return;
    }
    const core::Result<void> started = m_sound->startRecording();
    if (!started) {
        m_notebook->giveUpRecording(into);
        emit cannotRecord(QString::fromStdString(started.error().message));
        takeTrouble(QString::fromStdString(started.error().message));
        return;
    }
    m_recording = true;
    m_paused = false;
    m_recordedFor = 0;
    m_recordingInto = into;
    tellTheNotebook();
    emit workingChanged();
}

void RecordingViewModel::pauseRecording() {
    if (!m_recording || m_paused || m_sound == nullptr) {
        return;
    }
    m_sound->pauseRecording();
    m_paused = true;
    emit workingChanged();
}

void RecordingViewModel::carryOnRecording() {
    if (!m_recording || !m_paused || m_sound == nullptr) {
        return;
    }
    m_sound->carryOnRecording();
    m_paused = false;
    emit workingChanged();
}

void RecordingViewModel::recordOrPause() {
    if (!m_recording) {
        startRecording();
        return;
    }
    if (m_paused) {
        carryOnRecording();
    } else {
        pauseRecording();
    }
}

void RecordingViewModel::stopRecording() {
    if (!m_recording || m_sound == nullptr) {
        return;
    }
    m_recording = false;
    m_paused = false;
    tellTheNotebook();
    m_sound->stopRecording();
    emit workingChanged();
}

void RecordingViewModel::recordingReached(std::int64_t at) {
    if (!m_recording) {
        return;
    }
    m_recordedFor = at;
    tellTheNotebook();
    emit workingChanged();
}

void RecordingViewModel::recordingMade(std::vector<std::byte> sound, std::int64_t length) {
    m_recording = false;
    m_paused = false;
    const QString into = std::exchange(m_recordingInto, QString{});
    emit workingChanged();
    if (m_notebook.isNull() || into.isEmpty()) {
        return;
    }
    QByteArray held;
    held.resize(static_cast<qsizetype>(sound.size()));
    for (std::size_t step = 0; step < sound.size(); ++step) {
        held[static_cast<qsizetype>(step)] = static_cast<char>(sound[step]);
    }
    m_notebook->keepRecording(into, held, std::max<qint64>(length, m_recordedFor));
}

void RecordingViewModel::recordingFailed(const core::Error& why) {
    m_recording = false;
    m_paused = false;
    const QString into = std::exchange(m_recordingInto, QString{});
    tellTheNotebook();
    if (!m_notebook.isNull() && !into.isEmpty()) {
        m_notebook->giveUpRecording(into);
    }
    takeTrouble(QString::fromStdString(why.message));
    emit workingChanged();
    emit cannotRecord(QString::fromStdString(why.message));
}

void RecordingViewModel::play(const QString& recordingId) {
    if (m_sound == nullptr || m_notebook.isNull() || recordingId.isEmpty()) {
        return;
    }
    if (m_playingId == recordingId && !m_playing && m_playedTo > 0) {
        carryOnPlaying();
        return;
    }
    forgetTrouble();
    m_sound->stopPlaying();
    m_playing = false;
    m_playingId.clear();
    m_playedTo = 0;
    m_playingLength = 0;
    m_wantedId = recordingId;
    emit playingChanged();
    m_notebook->wantSound(recordingId);
}

void RecordingViewModel::pausePlaying() {
    if (m_sound == nullptr || !m_playing) {
        return;
    }
    m_sound->pausePlaying();
    m_playing = false;
    emit playingChanged();
}

void RecordingViewModel::carryOnPlaying() {
    if (m_sound == nullptr || m_playing || m_playingId.isEmpty()) {
        return;
    }
    m_sound->carryOnPlaying();
    m_playing = true;
    emit playingChanged();
}

void RecordingViewModel::playOrPause(const QString& recordingId) {
    if (m_playingId == recordingId && m_playing) {
        pausePlaying();
        return;
    }
    play(recordingId);
}

void RecordingViewModel::stopPlaying() {
    if (m_sound == nullptr) {
        return;
    }
    m_sound->stopPlaying();
    m_playing = false;
    m_playingId.clear();
    m_wantedId.clear();
    m_playedTo = 0;
    m_playingLength = 0;
    m_goingTo = -1;
    emit playingChanged();
}

void RecordingViewModel::goTo(qint64 at) {
    if (m_sound == nullptr) {
        return;
    }
    const qint64 wanted = std::max<qint64>(0, at);
    if (m_playingId.isEmpty()) {
        m_goingTo = wanted;
        return;
    }
    m_sound->goTo(wanted);
    m_playedTo = wanted;
    emit playingChanged();
}

void RecordingViewModel::playFromThing(const QString& thingId) {
    if (m_notebook.isNull() || thingId.isEmpty()) {
        return;
    }
    const QString recordingId = m_notebook->recordingOf(thingId);
    const qint64 at = m_notebook->momentOf(thingId);
    if (recordingId.isEmpty() || at < 0) {
        return;
    }
    if (m_playingId == recordingId) {
        goTo(at);
        carryOnPlaying();
        return;
    }
    m_goingTo = at;
    play(recordingId);
}

void RecordingViewModel::playingReached(std::int64_t at, std::int64_t length) {
    m_playedTo = at;
    m_playingLength = length;
    emit playingChanged();
}

void RecordingViewModel::playingStopped() {
    if (!m_playing && m_playingId.isEmpty()) {
        return;
    }
    m_playing = false;
    emit playingChanged();
}

void RecordingViewModel::playingFailed(const core::Error& why) {
    m_playing = false;
    m_playingId.clear();
    m_wantedId.clear();
    takeTrouble(QString::fromStdString(why.message));
    emit playingChanged();
}

QString RecordingViewModel::saidTime(qint64 milliseconds) {
    const qint64 shown = std::max<qint64>(0, milliseconds);
    const qint64 hours = shown / kHour;
    const qint64 minutes = (shown % kHour) / kMinute;
    const qint64 seconds = (shown % kMinute) / kSecond;
    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(twoFigures(minutes))
            .arg(twoFigures(seconds));
    }
    return QStringLiteral("%1:%2").arg(twoFigures(minutes), twoFigures(seconds));
}

}
