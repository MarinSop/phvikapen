#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QtQmlIntegration>

namespace phvikapen::app {

class AppInfo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString version READ version CONSTANT FINAL)
    Q_PROPERTY(QString plainFont READ plainFont CONSTANT FINAL)

public:
    explicit AppInfo(QObject* parent = nullptr);

    [[nodiscard]] QString version() const { return m_version; }

    // The face text wears when nobody has chosen one.
    [[nodiscard]] static QString plainFont();

    Q_INVOKABLE [[nodiscard]] static QString shortcutText(const QVariant& shortcut);

    Q_INVOKABLE [[nodiscard]] static QString keyName(int key);

    Q_INVOKABLE [[nodiscard]] static QUrl fileUrl(const QString& path);

private:
    QString m_version;
};

}
