#pragma once

#include "core/id/Uuid.hpp"
#include "core/model/PageStyle.hpp"

#include <string>
#include <vector>

namespace phvikapen::core {

struct PageInfo {
    Uuid id;
    std::string title;
    PageStyle style;

    friend bool operator==(const PageInfo&, const PageInfo&) = default;
};

struct SectionInfo {
    Uuid id;
    std::string title;
    std::vector<PageInfo> pages;

    friend bool operator==(const SectionInfo&, const SectionInfo&) = default;
};

struct NotebookOutline {
    std::string title;
    std::vector<SectionInfo> sections;

    friend bool operator==(const NotebookOutline&, const NotebookOutline&) = default;
};

}
