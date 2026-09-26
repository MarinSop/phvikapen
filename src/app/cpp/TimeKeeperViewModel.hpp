#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQmlIntegration>
#include <QtTypes>

namespace phvikapen::app {

// A clock kept beside the notes: one that counts down to nothing, and one that counts up from it.
// It runs on its own and touches no notebook, so it goes on keeping time whatever is being written.
class TimeKeeperViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Way way READ way WRITE setWay NOTIFY wayChanged FINAL)
    Q_PROPERTY(bool running READ running NOTIFY tick FINAL)
    Q_PROPERTY(bool started READ started NOTIFY tick FINAL)
    Q_PROPERTY(bool rang READ rang NOTIFY tick FINAL)
    Q_PROPERTY(qint64 gone READ gone NOTIFY tick FINAL)
    Q_PROPERTY(qint64 left READ left NOTIFY tick FINAL)
    Q_PROPERTY(qint64 wanted READ wanted WRITE setWanted NOTIFY wantedChanged FINAL)
    Q_PROPERTY(qreal howFar READ howFar NOTIFY tick FINAL)
    Q_PROPERTY(QString said READ said NOTIFY tick FINAL)

public:
    enum class Way : quint8 {
        // Counting down to nothing from a time that was set.
        Down,
        // Counting up from nothing.
        Up,
    };
    Q_ENUM(Way)

    static constexpr qint64 kSecond = 1000;
    static constexpr qint64 kMinute = 60 * kSecond;
    static constexpr qint64 kHour = 60 * kMinute;
    static constexpr qint64 kLongest = 24 * kHour;
    static constexpr qint64 kShortest = kSecond;
    static constexpr qint64 kDefaultWanted = 25 * kMinute;

    explicit TimeKeeperViewModel(QObject* parent = nullptr);

    [[nodiscard]] Way way() const { return m_way; }

    void setWay(Way way);

    [[nodiscard]] bool running() const { return m_beat.isActive(); }

    // Whether there is a count under way at all, whether or not it is running just now.
    [[nodiscard]] bool started() const { return running() || m_held > 0; }

    // Whether a countdown has reached nothing and is waiting to be seen.
    [[nodiscard]] bool rang() const { return m_rang; }

    [[nodiscard]] qint64 gone() const;
    [[nodiscard]] qint64 left() const;

    [[nodiscard]] qint64 wanted() const { return m_wanted; }

    void setWanted(qint64 milliseconds);

    // How far along a countdown is, from nothing to one. A stopwatch is never along at all.
    [[nodiscard]] qreal howFar() const;

    // The time as it is read out: hours only where there are any.
    [[nodiscard]] QString said() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void reset();

    // Started where it is not running, paused where it is: the one button a reader reaches for.
    Q_INVOKABLE void startOrPause();

    // The countdown set in the parts a reader thinks in rather than in milliseconds.
    Q_INVOKABLE void setWantedParts(int hours, int minutes, int seconds);

    Q_INVOKABLE [[nodiscard]] int wantedHours() const;
    Q_INVOKABLE [[nodiscard]] int wantedMinutes() const;
    Q_INVOKABLE [[nodiscard]] int wantedSeconds() const;

    // What the reader has seen; the ringing is put away until the next time.
    Q_INVOKABLE void seen();

signals:
    void wayChanged();
    void wantedChanged();
    void tick();
    // A countdown has reached nothing.
    void rangOut();

private:
    void beat();
    void remember() const;
    void recall();
    [[nodiscard]] qint64 counted() const;

    Way m_way{Way::Down};
    qint64 m_wanted{kDefaultWanted};
    // How much had gone by when the count was last paused.
    qint64 m_held{0};
    bool m_rang{false};
    QElapsedTimer m_since;
    QTimer m_beat;
};

}
