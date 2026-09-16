#pragma once

#include "platform/ink/qt/QtInkItem.hpp"

#include <QtQmlIntegration>

namespace phvikapen::app {

/// Exposes the Qt ink backend to QML as InkCanvas, so that the platform layer stays free of QML.
struct InkCanvasForeign {
    Q_GADGET
    QML_FOREIGN(phvikapen::platform::ink::QtInkItem)
    QML_NAMED_ELEMENT(InkCanvas)
};

} // namespace phvikapen::app
