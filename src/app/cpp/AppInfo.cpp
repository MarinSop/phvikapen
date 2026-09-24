#include "app/cpp/AppInfo.hpp"

#include "core/version.hpp"

#include <QFont>
#include <QGuiApplication>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <string_view>

namespace phvikapen::app {
namespace {

[[nodiscard]] bool typingInto(const QObject* holder) {
    return holder != nullptr
           && (holder->inherits("QQuickTextInput") || holder->inherits("QQuickTextEdit"));
}

[[nodiscard]] QString versionString() {
    constexpr std::string_view kText = core::version::kString;
    return QString::fromLatin1(kText.data(), static_cast<qsizetype>(kText.size()));
}

}

AppInfo::AppInfo(QObject* parent) : QObject(parent), m_version{versionString()} {
    if (QGuiApplication* const application = qGuiApp; application != nullptr) {
        connect(application, &QGuiApplication::focusObjectChanged, this, &AppInfo::typingChanged);
    }
}

QString AppInfo::keyName(int key) {
    return QKeySequence{key}.toString(QKeySequence::PortableText);
}

bool AppInfo::typing() {
    return typingInto(QGuiApplication::focusObject());
}

QString AppInfo::plainFont() {
    return QGuiApplication::font().family();
}

QUrl AppInfo::fileUrl(const QString& path) {
    return QUrl::fromLocalFile(path);
}

QString AppInfo::shortcutText(const QVariant& shortcut) {
    if (shortcut.typeId() == QMetaType::QString) {
        return QKeySequence{shortcut.toString()}.toString(QKeySequence::NativeText);
    }
    if (shortcut.canConvert<int>()) {
        return QKeySequence{static_cast<QKeySequence::StandardKey>(shortcut.toInt())}.toString(
            QKeySequence::NativeText);
    }
    return {};
}

}
