#include "app/cpp/AppInfo.hpp"

#include "core/version.hpp"

#include <QKeySequence>
#include <QString>
#include <QVariant>

#include <string_view>

namespace phvikapen::app {
namespace {

[[nodiscard]] QString versionString() {
    constexpr std::string_view kText = core::version::kString;
    return QString::fromLatin1(kText.data(), static_cast<qsizetype>(kText.size()));
}

}

AppInfo::AppInfo(QObject* parent) : QObject(parent), m_version{versionString()} {}

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
