#include <QCoreApplication>
#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtQuickTest/quicktest.h>

class Setup : public QObject {
    Q_OBJECT

public slots:

    static void applicationAvailable() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("PhvikaPenTests"));
        QCoreApplication::setApplicationName(QStringLiteral("PhvikaPenTests"));
        QSettings settings;
        settings.clear();
    }

    void qmlEngineAvailable(QQmlEngine* engine) {
        engine->rootContext()->setContextProperty(QStringLiteral("temporaryDirectory"),
                                                  m_directory.path());
    }

private:
    QTemporaryDir m_directory;
};

QUICK_TEST_MAIN_WITH_SETUP(phvikapen_qml, Setup)

#include "tst_main.moc"
