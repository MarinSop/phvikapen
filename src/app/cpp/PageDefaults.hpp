#pragma once

#include "core/model/PageStyle.hpp"

namespace phvikapen::app::defaults {

[[nodiscard]] core::PageStyle pageStyle();

void setPageStyle(const core::PageStyle& style);

}
