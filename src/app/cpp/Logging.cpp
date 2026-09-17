#include "app/cpp/Logging.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <QtLogging>

#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>

namespace phvikapen::app {
namespace {

constexpr std::size_t kMaxLogFileBytes = std::size_t{5} * 1024 * 1024;
constexpr std::size_t kMaxLogFiles = 3;
constexpr std::chrono::seconds kFlushInterval{2};

void forwardQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    const std::string text = message.toStdString();
    const std::string category = context.category != nullptr ? context.category : "qt";

    switch (type) {
    case QtDebugMsg:
        spdlog::debug("[{}] {}", category, text);
        break;
    case QtInfoMsg:
        spdlog::info("[{}] {}", category, text);
        break;
    case QtWarningMsg:
        spdlog::warn("[{}] {}", category, text);
        break;
    case QtCriticalMsg:
        spdlog::error("[{}] {}", category, text);
        break;
    case QtFatalMsg:
        spdlog::critical("[{}] {}", category, text);
        break;
    }
}

}

void initializeLogging() {
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
    if (!QDir().mkpath(directory)) {
        qWarning("Could not create the log directory %s", qUtf8Printable(directory));
        return;
    }

    const QString file = directory + "/phvikapen.log";
    try {
        auto logger = spdlog::rotating_logger_mt("phvikapen", file.toStdString(), kMaxLogFileBytes,
                                                 kMaxLogFiles);
        spdlog::set_default_logger(std::move(logger));
        spdlog::flush_on(spdlog::level::warn);
        spdlog::flush_every(kFlushInterval);
    } catch (const std::exception& error) {
        qWarning("Could not open the log file %s: %s", qUtf8Printable(file), error.what());
        return;
    }

    qInstallMessageHandler(forwardQtMessage);
    spdlog::info("Log file: {}", file.toStdString());
}

}
