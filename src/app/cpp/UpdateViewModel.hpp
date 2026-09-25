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
    Q_PROPERTY(int howFarAlong READ howFarAlong NOTIFY howFarAlongChanged FINAL)
    Q_PROPERTY(
        bool testVersions READ testVersions WRITE setTestVersions NOTIFY testVersionsChanged FINAL)
    Q_PROPERTY(QString skippedVersion READ skippedVersion WRITE setSkippedVersion NOTIFY
                   skippedVersionChanged FINAL)
    Q_PROPERTY(bool worthOffering READ worthOffering NOTIFY stateChanged FINAL)

public:
    enum class State : quint8 {
        Idle,
        Looking,
        Available,
        Getting,
        Ready,
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

    [[nodiscard]] bool busy() const {
        return m_state == State::Looking || m_state == State::Getting;
    }

    [[nodiscard]] int howFarAlong() const { return m_howFarAlong; }

    [[nodiscard]] bool testVersions() const { return m_testVersions; }

    void setTestVersions(bool wanted);

    [[nodiscard]] QString skippedVersion() const { return m_skippedVersion; }

    void setSkippedVersion(const QString& version);

    [[nodiscard]] bool worthOffering() const {
        return m_state == State::Available && m_version != m_skippedVersion;
    }

    Q_INVOKABLE void check();

    Q_INVOKABLE void get();

    Q_INVOKABLE void restartNow();

    Q_INVOKABLE void skipThisVersion();

signals:
    void stateChanged();
    void howFarAlongChanged();
    void testVersionsChanged();
    void skippedVersionChanged();
    void failed(const QString& message);
    void restartWanted();
    void updateFound();

private:
    void show(State state, QString version);
    void showFailure(State state, const QString& message);

    void showHowFarAlong(int howFar);

    std::unique_ptr<platform::update::IUpdater> m_updater;
    std::jthread m_worker;
    State m_state{State::Idle};
    QString m_version;
    QString m_skippedVersion;
    int m_howFarAlong{0};
    bool m_testVersions{false};
    bool m_reopen{false};
};

}
