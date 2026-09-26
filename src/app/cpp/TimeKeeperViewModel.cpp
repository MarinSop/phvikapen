#include "app/cpp/TimeKeeperViewModel.hpp"

#include <QSettings>
#include <QString>

#include <algorithm>

namespace phvikapen::app {
namespace {

constexpr auto kWaySetting = "timeKeeper/way";
constexpr auto kWantedSetting = "timeKeeper/wanted";
constexpr int kBeat = 100;
constexpr int kTens = 10;

[[nodiscard]] QString twoFigures(qint64 of) {
    return QStringLiteral("%1").arg(of, 2, kTens, QLatin1Char{'0'});
}

}

TimeKeeperViewModel::TimeKeeperViewModel(QObject* parent) : QObject(parent) {
    recall();
    m_beat.setInterval(kBeat);
    m_beat.setTimerType(Qt::PreciseTimer);
    connect(&m_beat, &QTimer::timeout, this, &TimeKeeperViewModel::beat);
}

void TimeKeeperViewModel::recall() {
    const QSettings settings;
    const int way = settings.value(kWaySetting, static_cast<int>(Way::Down)).toInt();
    m_way = way == static_cast<int>(Way::Up) ? Way::Up : Way::Down;
    m_wanted = std::clamp(settings.value(kWantedSetting, kDefaultWanted).toLongLong(), kShortest,
                          kLongest);
}

void TimeKeeperViewModel::remember() const {
    QSettings settings;
    settings.setValue(kWaySetting, static_cast<int>(m_way));
    settings.setValue(kWantedSetting, m_wanted);
}

void TimeKeeperViewModel::setWay(Way way) {
    if (way == m_way) {
        return;
    }
    reset();
    m_way = way;
    remember();
    emit wayChanged();
    emit tick();
}

void TimeKeeperViewModel::setWanted(qint64 milliseconds) {
    const qint64 held = std::clamp(milliseconds, kShortest, kLongest);
    if (held == m_wanted) {
        return;
    }
    m_wanted = held;
    remember();
    emit wantedChanged();
    emit tick();
}

void TimeKeeperViewModel::setWantedParts(int hours, int minutes, int seconds) {
    const qint64 asked = (static_cast<qint64>(std::max(0, hours)) * kHour)
                         + (static_cast<qint64>(std::max(0, minutes)) * kMinute)
                         + (static_cast<qint64>(std::max(0, seconds)) * kSecond);
    setWanted(asked);
}

int TimeKeeperViewModel::wantedHours() const {
    return static_cast<int>(m_wanted / kHour);
}

int TimeKeeperViewModel::wantedMinutes() const {
    return static_cast<int>((m_wanted % kHour) / kMinute);
}

int TimeKeeperViewModel::wantedSeconds() const {
    return static_cast<int>((m_wanted % kMinute) / kSecond);
}

qint64 TimeKeeperViewModel::counted() const {
    const qint64 running = m_since.isValid() && m_beat.isActive() ? m_since.elapsed() : 0;
    return m_held + running;
}

qint64 TimeKeeperViewModel::gone() const {
    const qint64 counted = TimeKeeperViewModel::counted();
    return m_way == Way::Down ? std::min(counted, m_wanted) : counted;
}

qint64 TimeKeeperViewModel::left() const {
    return m_way == Way::Down ? std::max<qint64>(0, m_wanted - counted()) : counted();
}

qreal TimeKeeperViewModel::howFar() const {
    if (m_way != Way::Down || m_wanted <= 0) {
        return 0.0;
    }
    return std::clamp(static_cast<qreal>(gone()) / static_cast<qreal>(m_wanted), 0.0, 1.0);
}

QString TimeKeeperViewModel::said() const {
    const qint64 shown = m_way == Way::Down ? left() : gone();
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

void TimeKeeperViewModel::start() {
    if (m_beat.isActive()) {
        return;
    }
    if (m_way == Way::Down && left() <= 0) {
        m_held = 0;
    }
    m_rang = false;
    m_since.restart();
    m_beat.start();
    emit tick();
}

void TimeKeeperViewModel::pause() {
    if (!m_beat.isActive()) {
        return;
    }
    m_held += m_since.isValid() ? m_since.elapsed() : 0;
    m_beat.stop();
    emit tick();
}

void TimeKeeperViewModel::startOrPause() {
    if (m_beat.isActive()) {
        pause();
    } else {
        start();
    }
}

void TimeKeeperViewModel::reset() {
    m_beat.stop();
    m_held = 0;
    m_rang = false;
    m_since.invalidate();
    emit tick();
}

void TimeKeeperViewModel::seen() {
    if (!m_rang) {
        return;
    }
    m_rang = false;
    emit tick();
}

void TimeKeeperViewModel::beat() {
    if (m_way == Way::Down && left() <= 0) {
        m_held = m_wanted;
        m_beat.stop();
        m_since.invalidate();
        m_rang = true;
        emit tick();
        emit rangOut();
        return;
    }
    emit tick();
}

}
