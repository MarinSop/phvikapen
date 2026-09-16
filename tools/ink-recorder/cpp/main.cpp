#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>

#include <cstdlib>

int main(int argc, char* argv[]) {
    const QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("PhvikaPen Ink Recorder"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("InkRecorder", "Main");

    return QGuiApplication::exec();
}
