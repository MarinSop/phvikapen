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
constexpr auto kCustomWidthKey = "pages/customWidth";
constexpr auto kCustomHeightKey = "pages/customHeight";
constexpr auto kPaperColorKey = "pages/paperColor";
constexpr auto kLineColorKey = "pages/lineColor";
constexpr auto kMarginColorKey = "pages/marginColor";
constexpr auto kLineWidthKey = "pages/lineWidth";
constexpr auto kMarginAtKey = "pages/marginAt";
constexpr auto kMarginKey = "pages/margin";

constexpr float kOwnWidth = core::millimeters(210.0F);
constexpr float kOwnHeight = core::millimeters(297.0F);

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
        .paper = readEnum(settings, kPaperKey, usual.paper, core::Paper::Custom),
        .orientation =
            readEnum(settings, kOrientationKey, usual.orientation, core::Orientation::Landscape),
        .background =
            readEnum(settings, kBackgroundKey, usual.background, core::Background::Dotted),
        .spacing = static_cast<float>(settings.value(kSpacingKey, usual.spacing).toDouble()),
        .customWidth = static_cast<float>(settings.value(kCustomWidthKey, kOwnWidth).toDouble()),
        .customHeight = static_cast<float>(settings.value(kCustomHeightKey, kOwnHeight).toDouble()),
        .paperColor =
            core::unpacked(settings.value(kPaperColorKey, core::packed(usual.paperColor)).toUInt()),
        .lineColor =
            core::unpacked(settings.value(kLineColorKey, core::packed(usual.lineColor)).toUInt()),
        .marginColor = core::unpacked(
            settings.value(kMarginColorKey, core::packed(usual.marginColor)).toUInt()),
        .lineWidth = static_cast<float>(settings.value(kLineWidthKey, usual.lineWidth).toDouble()),
        .marginAt = static_cast<float>(settings.value(kMarginAtKey, usual.marginAt).toDouble()),
        .margin = settings.value(kMarginKey, usual.margin).toBool(),
    });
}

void setPageStyle(const core::PageStyle& style) {
    QSettings settings;
    settings.setValue(kPaperKey, static_cast<int>(style.paper));
    settings.setValue(kOrientationKey, static_cast<int>(style.orientation));
    settings.setValue(kBackgroundKey, static_cast<int>(style.background));
    settings.setValue(kSpacingKey, static_cast<double>(style.spacing));
    settings.setValue(kCustomWidthKey, static_cast<double>(style.customWidth));
    settings.setValue(kCustomHeightKey, static_cast<double>(style.customHeight));
    settings.setValue(kPaperColorKey, core::packed(style.paperColor));
    settings.setValue(kLineColorKey, core::packed(style.lineColor));
    settings.setValue(kMarginColorKey, core::packed(style.marginColor));
    settings.setValue(kLineWidthKey, static_cast<double>(style.lineWidth));
    settings.setValue(kMarginAtKey, static_cast<double>(style.marginAt));
    settings.setValue(kMarginKey, style.margin);
}

}
