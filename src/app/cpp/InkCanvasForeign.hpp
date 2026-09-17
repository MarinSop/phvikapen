#pragma once

#include "platform/ink/qt/QtInkItem.hpp"

#include <QtQmlIntegration>

namespace phvikapen::app {

struct InkCanvasForeign {
    Q_GADGET
    QML_FOREIGN(phvikapen::platform::ink::QtInkItem)
    QML_NAMED_ELEMENT(InkCanvas)
};

}
