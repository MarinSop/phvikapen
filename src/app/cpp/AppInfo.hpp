#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace phvikapen::app {

class AppInfo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString version READ version CONSTANT FINAL)

public:
    explicit AppInfo(QObject* parent = nullptr);

    [[nodiscard]] QString version() const { return m_version; }

private:
    QString m_version;
};

}
