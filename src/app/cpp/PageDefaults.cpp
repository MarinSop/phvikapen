#include "app/cpp/PageDefaults.hpp"

#include "core/model/PageStyle.hpp"

#include <QSettings>
#include <QVariant>

#include <cstdint>

namespace phvikapen::app::defaults {
namespace {

constexpr auto kPaperKey = "pages/paper";
constexpr auto kOrientationKey = "pages/orientation";
constexpr auto kBackgroundKey = "pages/background";
constexpr auto kSpacingKey = "pages/spacing";

template <typename Value>
[[nodiscard]] Value readEnum(const QSettings& settings, const char* key, Value fallback,
                             Value highest) {
    const int stored = settings.value(key, static_cast<int>(fallback)).toInt();
    if (stored < 0 || stored > static_cast<int>(highest)) {
        return fallback;
    }
    return static_cast<Value>(stored);
}

}

core::PageStyle pageStyle() {
    const QSettings settings;
    const core::PageStyle usual;
    return core::normalized(core::PageStyle{
        .paper = readEnum(settings, kPaperKey, usual.paper, core::Paper::Legal),
        .orientation =
            readEnum(settings, kOrientationKey, usual.orientation, core::Orientation::Landscape),
        .background =
            readEnum(settings, kBackgroundKey, usual.background, core::Background::Dotted),
        .spacing = static_cast<float>(settings.value(kSpacingKey, usual.spacing).toDouble()),
        .customWidth = 0.0F,
        .customHeight = 0.0F,
    });
}

void setPageStyle(const core::PageStyle& style) {
    QSettings settings;
    settings.setValue(kPaperKey, static_cast<int>(style.paper));
    settings.setValue(kOrientationKey, static_cast<int>(style.orientation));
    settings.setValue(kBackgroundKey, static_cast<int>(style.background));
    settings.setValue(kSpacingKey, static_cast<double>(style.spacing));
}

}
