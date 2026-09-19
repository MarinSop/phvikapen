#include "app/cpp/Logging.hpp"
#include "app/cpp/SystemMenus.hpp"
#include "app/cpp/Thumbnails.hpp"
#include "core/version.hpp"
#include "platform/update/StartupHook.hpp"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>

#include <cstdlib>

int main(int argc, char* argv[]) {
    // Must run first: it may restart or end the process.
    phvikapen::platform::update::runStartupHook();
    phvikapen::app::keepSystemItemsOutOfMenus();
    // Every position the pen reports is ink; none may be merged away.
    QGuiApplication::setAttribute(Qt::AA_CompressTabletEvents, false);

    const QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("PhvikaPen"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("phvikapen.app"));
    QGuiApplication::setApplicationVersion(
        QString::fromLatin1(phvikapen::core::version::kString.data(),
                            static_cast<qsizetype>(phvikapen::core::version::kString.size())));

    QGuiApplication::setWindowIcon(QIcon{QStringLiteral(":/brand/logo-256.png")});

    phvikapen::app::initializeLogging();

    QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("pages"), phvikapen::app::thumbnails::makeProvider());
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("PhvikaPen", "Main");

    return QGuiApplication::exec();
}
