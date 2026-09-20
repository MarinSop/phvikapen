#include "app/cpp/UpdateViewModel.hpp"

#include "core/Error.hpp"
#include "platform/update/IUpdater.hpp"
#include "platform/update/VelopackUpdater.hpp"

#include <QMetaObject>
#include <QString>
#include <QtLogging>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace phvikapen::app {
namespace {

using platform::update::UpdateInfo;

[[nodiscard]] QString toQString(const std::string& text) {
    return QString::fromStdString(text);
}

}

UpdateViewModel::UpdateViewModel(QObject* parent) : QObject(parent) {}

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

void UpdateViewModel::install() {
    if (m_state != State::Available || !m_updater) {
        return;
    }
    show(State::Installing, m_version);
    if (m_worker.joinable()) {
        m_worker.join();
    }

    const auto update =
        std::make_shared<const UpdateInfo>(UpdateInfo{.version = m_version.toStdString()});
    m_worker = std::jthread{[this, update] {
        try {
            core::Result<void> installed = m_updater->downloadAndRestart(*update);
            QMetaObject::invokeMethod(
                this,
                [this, installed = std::move(installed)] {
                    if (!installed) {
                        showFailure(State::Unavailable, toQString(installed.error().message));
                        return;
                    }
                    emit restartWanted();
                },
                Qt::QueuedConnection);
        } catch (...) {
            qWarning("Installing the update stopped unexpectedly");
        }
    }};
}

}
