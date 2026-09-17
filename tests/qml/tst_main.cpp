#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QtQuickTest/quicktest.h>

class Setup : public QObject {
    Q_OBJECT

public slots:

    void qmlEngineAvailable(QQmlEngine* engine) {
        engine->rootContext()->setContextProperty(QStringLiteral("temporaryDirectory"),
                                                  m_directory.path());
    }

private:
    QTemporaryDir m_directory;
};

QUICK_TEST_MAIN_WITH_SETUP(phvikapen_qml, Setup)

#include "tst_main.moc"
