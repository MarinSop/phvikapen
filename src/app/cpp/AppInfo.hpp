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
    Q_PROPERTY(bool typing READ typing NOTIFY typingChanged FINAL)

public:
    explicit AppInfo(QObject* parent = nullptr);

    [[nodiscard]] QString version() const { return m_version; }

    // The face text wears when nobody has chosen one.
    [[nodiscard]] static QString plainFont();

    // For showing a key beside a command. What a command answers to is asked of the settings,
    // never of this, because what is shown here is written the way the platform writes it and
    // cannot be read back.
    Q_INVOKABLE [[nodiscard]] static QString shortcutText(const QVariant& shortcut);

    Q_INVOKABLE [[nodiscard]] static QString keyName(int key);

    // Whether whatever holds the keyboard is somewhere words are being typed, so that a command
    // with a plain letter for a key keeps out of the way of the letter.
    [[nodiscard]] static bool typing();

    Q_INVOKABLE [[nodiscard]] static QUrl fileUrl(const QString& path);

signals:
    void typingChanged();

private:
    QString m_version;
};

}
