#pragma once

#include "platform/audio/ISound.hpp"
#include "platform/speech/ISpeech.hpp"

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration>
#include <QtTypes>

#include <cstddef>
#include <memory>
#include <vector>

namespace phvikapen::app {

class NotebookViewModel;

// Making a recording and playing one back, as the window asks for them. What was recorded is
// handed to the notebook to keep; what is kept is asked back from it to play.
class RecordingViewModel : public QObject, private platform::audio::ISoundSink {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(phvikapen::app::NotebookViewModel* notebook READ notebook WRITE setNotebook NOTIFY
                   notebookChanged FINAL)
    Q_PROPERTY(bool canRecord READ canRecord NOTIFY workingChanged FINAL)
    Q_PROPERTY(bool recording READ recording NOTIFY workingChanged FINAL)
    Q_PROPERTY(bool paused READ paused NOTIFY workingChanged FINAL)
    Q_PROPERTY(qint64 recordedFor READ recordedFor NOTIFY workingChanged FINAL)
    Q_PROPERTY(QString playingId READ playingId NOTIFY playingChanged FINAL)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged FINAL)
    Q_PROPERTY(qint64 playedTo READ playedTo NOTIFY playingChanged FINAL)
    Q_PROPERTY(qint64 playingLength READ playingLength NOTIFY playingChanged FINAL)
    Q_PROPERTY(QString trouble READ trouble NOTIFY troubleChanged FINAL)
    Q_PROPERTY(bool canRead READ canRead CONSTANT FINAL)
    Q_PROPERTY(QString readingNow READ readingNow NOTIFY readingChanged FINAL)
    Q_PROPERTY(QStringList languages READ languages CONSTANT FINAL)

public:
    explicit RecordingViewModel(QObject* parent = nullptr);
    ~RecordingViewModel() override;

    RecordingViewModel(const RecordingViewModel&) = delete;
    RecordingViewModel& operator=(const RecordingViewModel&) = delete;
    RecordingViewModel(RecordingViewModel&&) = delete;
    RecordingViewModel& operator=(RecordingViewModel&&) = delete;

    [[nodiscard]] NotebookViewModel* notebook() const;
    void setNotebook(NotebookViewModel* notebook);

    [[nodiscard]] bool canRecord() const;

    [[nodiscard]] bool recording() const { return m_recording; }

    [[nodiscard]] bool paused() const { return m_paused; }

    [[nodiscard]] qint64 recordedFor() const { return m_recordedFor; }

    [[nodiscard]] QString playingId() const { return m_playingId; }

    [[nodiscard]] bool playing() const { return m_playing; }

    [[nodiscard]] qint64 playedTo() const { return m_playedTo; }

    [[nodiscard]] qint64 playingLength() const { return m_playingLength; }

    [[nodiscard]] QString trouble() const { return m_trouble; }

    // Whether this machine can read a recording back as words at all.
    [[nodiscard]] bool canRead() const { return m_speech != nullptr; }

    // Which recording is being read just now, empty where none is.
    [[nodiscard]] QString readingNow() const { return m_readingNow; }

    [[nodiscard]] QStringList languages() const;

    // Reads a recording back as words. What comes back is kept with the recording.
    Q_INVOKABLE void readWhatWasSaid(const QString& recordingId, const QString& language);

    Q_INVOKABLE void giveUpReading();

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void pauseRecording();
    Q_INVOKABLE void carryOnRecording();
    Q_INVOKABLE void stopRecording();

    // Starts the recording where it is not running, and pauses it where it is.
    Q_INVOKABLE void recordOrPause();

    Q_INVOKABLE void play(const QString& recordingId);
    Q_INVOKABLE void pausePlaying();
    Q_INVOKABLE void carryOnPlaying();
    Q_INVOKABLE void stopPlaying();

    // Starts playing where nothing is playing, pauses where this one is, and begins another where
    // a different one is.
    Q_INVOKABLE void playOrPause(const QString& recordingId);

    Q_INVOKABLE void goTo(qint64 at);

    // Plays from the moment a thing on the page was written, whichever recording that was.
    Q_INVOKABLE void playFromThing(const QString& thingId);

    Q_INVOKABLE void forgetTrouble();

    // The time as it is read out.
    Q_INVOKABLE [[nodiscard]] static QString saidTime(qint64 milliseconds);

signals:
    void notebookChanged();
    void workingChanged();
    void playingChanged();
    void troubleChanged();
    void readingChanged();
    // A recording was asked to be read but nothing on this machine can read speech.
    void cannotRead(const QString& why);
    // A recording was asked for but nothing on this machine can make one.
    void cannotRecord(const QString& why);

private:
    void recordingReached(std::int64_t at) override;
    void recordingMade(std::vector<std::byte> sound, std::int64_t length) override;
    void recordingFailed(const core::Error& why) override;
    void playingReached(std::int64_t at, std::int64_t length) override;
    void playingStopped() override;
    void playingFailed(const core::Error& why) override;

    void takeTrouble(const QString& why);
    void tellTheNotebook();
    void hearWhatWasSaid(const QString& recordingId, const QByteArray& sound);
    void soundArrived(const QString& recordingId, const QByteArray& sound);
    void soundNotThere(const QString& recordingId);

    std::unique_ptr<platform::audio::ISound> m_sound;
    std::unique_ptr<platform::speech::ISpeech> m_speech;
    QString m_readingNow;
    QString m_readingLanguage;
    QPointer<NotebookViewModel> m_notebook;
    bool m_recording{false};
    bool m_paused{false};
    qint64 m_recordedFor{0};
    QString m_recordingInto;
    QString m_playingId;
    QString m_wantedId;
    bool m_playing{false};
    qint64 m_playedTo{0};
    qint64 m_playingLength{0};
    qint64 m_goingTo{-1};
    QString m_trouble;
};

}
