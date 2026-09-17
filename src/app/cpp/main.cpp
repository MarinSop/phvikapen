#include "app/cpp/Logging.hpp"
#include "core/version.hpp"
#include "platform/update/StartupHook.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>

#include <cstdlib>

int main(int argc, char* argv[]) {
    // Must run first: it may restart or end the process.
    phvikapen::platform::update::runStartupHook();

    const QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("PhvikaPen"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("phvikapen.app"));
    QGuiApplication::setApplicationVersion(
        QString::fromLatin1(phvikapen::core::version::kString.data(),
                            static_cast<qsizetype>(phvikapen::core::version::kString.size())));

    phvikapen::app::initializeLogging();

    QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("PhvikaPen", "Main");

    return QGuiApplication::exec();
}
