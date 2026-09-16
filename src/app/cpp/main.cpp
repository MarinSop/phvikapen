#include "app/cpp/Logging.hpp"
#include "core/version.hpp"
#include "platform/update/StartupHook.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>

#include <cstdlib>

int main(int argc, char* argv[]) {
    // Installer and updater hooks run first; this call may restart or end the process.
    phvikapen::platform::update::runStartupHook();

    const QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("PhvikaPen"));
    QGuiApplication::setApplicationVersion(
        QString::fromLatin1(phvikapen::core::version::kString.data(),
                            static_cast<qsizetype>(phvikapen::core::version::kString.size())));

    phvikapen::app::initializeLogging();

    // A Windows 11 look, available on every platform Qt supports.
    QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("PhvikaPen", "Main");

    return QGuiApplication::exec();
}
