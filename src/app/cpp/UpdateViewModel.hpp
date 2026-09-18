#pragma once

#include "platform/update/IUpdater.hpp"

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

#include <memory>
#include <thread>

namespace phvikapen::app {

class UpdateViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString version READ version NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged FINAL)

public:
    enum class State : quint8 {
        Idle,
        Looking,
        Available,
        Installing,
        UpToDate,
        Unavailable,
    };
    Q_ENUM(State)

    explicit UpdateViewModel(QObject* parent = nullptr);
    ~UpdateViewModel() override;

    UpdateViewModel(const UpdateViewModel&) = delete;
    UpdateViewModel& operator=(const UpdateViewModel&) = delete;
    UpdateViewModel(UpdateViewModel&&) = delete;
    UpdateViewModel& operator=(UpdateViewModel&&) = delete;

    [[nodiscard]] State state() const { return m_state; }

    [[nodiscard]] QString version() const { return m_version; }

    [[nodiscard]] bool busy() const { return m_state == State::Looking || m_state == State::Installing; }

    Q_INVOKABLE void check();

    Q_INVOKABLE void install();

signals:
    void stateChanged();
    void failed(const QString& message);
    void restartWanted();

private:
    void show(State state, QString version);
    void showFailure(State state, const QString& message);

    std::unique_ptr<platform::update::IUpdater> m_updater;
    std::jthread m_worker;
    State m_state{State::Idle};
    QString m_version;
};

}
