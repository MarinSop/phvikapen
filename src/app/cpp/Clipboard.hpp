#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace phvikapen::app {

// What is on the clipboard of the machine, for the parts of the window that hand a reader words to
// take somewhere else: an answer, the text read out of a picture, what was said in a recording.
class Clipboard : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Clipboard(QObject* parent = nullptr);

    Q_INVOKABLE static void put(const QString& text);

    Q_INVOKABLE [[nodiscard]] static QString taken();

    Q_INVOKABLE [[nodiscard]] static bool holdsText();
};

}
