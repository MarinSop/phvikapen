#include "app/cpp/UpdateViewModel.hpp"

#include "core/Error.hpp"
#include "platform/update/IUpdater.hpp"
#include "platform/update/VelopackUpdater.hpp"

#include <QMetaObject>
#include <QSettings>
#include <QString>
#include <QtLogging>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace phvikapen::app {
namespace {

using platform::update::UpdateInfo;

constexpr auto kSkippedSetting = "updates/skipped";
constexpr int kWholeWay = 100;

[[nodiscard]] QString toQString(const std::string& text) {
    return QString::fromStdString(text);
}

}

UpdateViewModel::UpdateViewModel(QObject* parent) : QObject(parent) {
    const QSettings settings;
    m_skippedVersion = settings.value(kSkippedSetting).toString();
}

UpdateViewModel::~UpdateViewModel() {
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void UpdateViewModel::show(State state, QString version) {
    m_state = state;
    m_version = std::move(version);
    emit stateChanged();
}

void UpdateViewModel::showHowFarAlong(int howFar) {
    const int wanted = std::clamp(howFar, 0, kWholeWay);
    if (wanted == m_howFarAlong) {
        return;
    }
    m_howFarAlong = wanted;
    emit howFarAlongChanged();
}

void UpdateViewModel::setSkippedVersion(const QString& version) {
    if (version == m_skippedVersion) {
        return;
    }
    m_skippedVersion = version;
    QSettings settings;
    settings.setValue(kSkippedSetting, m_skippedVersion);
    emit skippedVersionChanged();
    emit stateChanged();
}

void UpdateViewModel::skipThisVersion() {
    setSkippedVersion(m_version);
}

void UpdateViewModel::showFailure(State state, const QString& message) {
    show(state, QString{});
    qWarning("%s", qUtf8Printable(message));
    emit failed(message);
}

void UpdateViewModel::setTestVersions(bool wanted) {
    if (m_testVersions == wanted) {
        return;
    }
    m_testVersions = wanted;
    // The place to look is settled when the updater is opened, so it is opened again.
    m_reopen = true;
    emit testVersionsChanged();
}

void UpdateViewModel::check() {
    if (busy()) {
        return;
    }
    show(State::Looking, m_version);
    if (m_worker.joinable()) {
        m_worker.join();
    }
    if (m_reopen) {
        m_updater.reset();
        m_reopen = false;
    }

    m_worker = std::jthread{[this, testVersions = m_testVersions] {
        try {
            if (!m_updater) {
                core::Result<std::unique_ptr<platform::update::VelopackUpdater>> opened =
                    platform::update::VelopackUpdater::open({}, testVersions);
                if (!opened) {
                    const QString message = toQString(opened.error().message);
                    QMetaObject::invokeMethod(
                        this, [this, message] { showFailure(State::Unavailable, message); },
                        Qt::QueuedConnection);
                    return;
                }
                m_updater = std::move(*opened);
            }

            core::Result<std::optional<UpdateInfo>> found = m_updater->checkForUpdates();
            QMetaObject::invokeMethod(
                this,
                [this, found = std::move(found)] {
                    if (!found) {
                        showFailure(State::Unavailable, toQString(found.error().message));
                    } else if (*found) {
                        show(State::Available, toQString((*found)->version));
                        if (worthOffering()) {
                            emit updateFound();
                        }
                    } else {
                        show(State::UpToDate, QString{});
                    }
                },
                Qt::QueuedConnection);
        } catch (...) {
            qWarning("Looking for updates stopped unexpectedly");
        }
    }};
}

void UpdateViewModel::get() {
    if (m_state != State::Available || !m_updater) {
        return;
    }
    showHowFarAlong(0);
    show(State::Getting, m_version);
    if (m_worker.joinable()) {
        m_worker.join();
    }

    const auto update =
        std::make_shared<const UpdateInfo>(UpdateInfo{.version = m_version.toStdString()});
    m_worker = std::jthread{[this, update] {
        try {
            const platform::update::HowFarAlong told = [this](int howFar) {
                QMetaObject::invokeMethod(
                    this, [this, howFar] { showHowFarAlong(howFar); }, Qt::QueuedConnection);
            };
            core::Result<void> got = m_updater->download(*update, told);
            QMetaObject::invokeMethod(
                this,
                [this, update, got = std::move(got)] {
                    if (!got) {
                        showFailure(State::Unavailable, toQString(got.error().message));
                        return;
                    }
                    showHowFarAlong(kWholeWay);
                    show(State::Ready, toQString(update->version));
                },
                Qt::QueuedConnection);
        } catch (...) {
            qWarning("Getting the update stopped unexpectedly");
        }
    }};
}

void UpdateViewModel::restartNow() {
    if (m_state != State::Ready || !m_updater) {
        return;
    }
    const core::Result<void> applied =
        m_updater->applyAndRestart(UpdateInfo{.version = m_version.toStdString()});
    if (!applied) {
        showFailure(State::Unavailable, toQString(applied.error().message));
        return;
    }
    emit restartWanted();
}

}
