#include "app/cpp/AppInfo.hpp"

#include "core/version.hpp"

#include <QString>

#include <string_view>

namespace phvikapen::app {
namespace {

[[nodiscard]] QString versionString() {
    constexpr std::string_view kText = core::version::kString;
    return QString::fromLatin1(kText.data(), static_cast<qsizetype>(kText.size()));
}

}

AppInfo::AppInfo(QObject* parent) : QObject(parent), m_version{versionString()} {}

}
