#include "app/cpp/NotebookViewModel.hpp"

#include "app/cpp/OutlineModels.hpp"
#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/undo/OutlineCommands.hpp"
#include "core/undo/StrokeCommands.hpp"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>
#include <QString>
#include <QtLogging>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr auto kDefaultNotebookName = "default.phvika";

[[nodiscard]] QString notebookDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/notebooks";
}

[[nodiscard]] std::optional<std::size_t> checkedIndex(int index, std::size_t size) noexcept {
    if (index < 0 || std::cmp_greater_equal(index, size)) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(index);
}

[[nodiscard]] std::size_t lastIndex(std::size_t size) noexcept {
    return size == 0 ? 0 : size - 1;
}

}

NotebookViewModel::NotebookViewModel(QObject* parent) : QObject(parent) {}

NotebookViewModel::NotebookViewModel(QString path, QString startPage, QObject* parent)
    : QObject(parent), m_notebookPath{std::move(path)}, m_startPage{std::move(startPage)},
      m_completed{true} {
    openNotebook();
}

QString NotebookViewModel::name() const {
    return QFileInfo{m_notebookPath}.completeBaseName();
}

QString NotebookViewModel::currentPageId() const {
    return m_currentPage.isNil() ? QString{} : QString::fromStdString(m_currentPage.toString());
}

void NotebookViewModel::setStartPage(const QString& pageId) {
    if (m_startPage == pageId) {
        return;
    }
    m_startPage = pageId;
    emit startPageChanged();
}

bool NotebookViewModel::renameTo(const QString& path) {
    if (path == m_notebookPath) {
        return true;
    }
    m_startPage = currentPageId();
    m_storage.reset();

    const std::filesystem::path from{m_notebookPath.toStdU16String()};
    const std::filesystem::path to{path.toStdU16String()};
    std::error_code failure;
    std::filesystem::rename(from, to, failure);
    if (failure) {
        reportError(tr("Could not rename %1").arg(m_notebookPath));
        openNotebook();
        return false;
    }
    for (const auto* suffix : {"-wal", "-shm"}) {
        std::error_code ignored;
        std::filesystem::rename(from.native() + suffix, to.native() + suffix, ignored);
    }

    m_notebookPath = path;
    emit notebookPathChanged();
    openNotebook();
    return true;
}

NotebookViewModel::~NotebookViewModel() {
    if (!m_canvas.isNull()) {
        m_canvas->setSink(nullptr);
    }
}

void NotebookViewModel::componentComplete() {
    m_completed = true;
    openNotebook();
}

void NotebookViewModel::setNotebookPath(const QString& path) {
    if (m_notebookPath == path) {
        return;
    }
    m_notebookPath = path;
    emit notebookPathChanged();
    if (m_completed) {
        openNotebook();
    }
}

void NotebookViewModel::openNotebook() {
    m_history.clear();
    m_erasing.clear();
    m_storage.reset();
    m_pages.clear();
    m_outline = core::Outline{};
    m_currentPage = core::Uuid{};
    const std::uint64_t opening = ++m_opening;
    setLoaded(false);
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit errorMessageChanged();
    }
    publishOutline();
    emit historyChanged();
    emit currentPageChanged();
    emit pageStyleChanged();
    emit pageChanged();
    refreshCanvas();

    QString file = m_notebookPath;
    if (file.isEmpty()) {
        const QString directory = notebookDirectory();
        if (!QDir().mkpath(directory)) {
            reportError(tr("Could not create %1").arg(directory));
            return;
        }
        file = directory + "/" + kDefaultNotebookName;
    }

    const std::filesystem::path path{file.toStdU16String()};
    m_storage.emplace(path, [this](const core::Error& error) {
        QMetaObject::invokeMethod(
            this, [this, message = QString::fromStdString(error.message)] { reportError(message); },
            Qt::QueuedConnection);
    });
    m_storage->loadOutline([this, opening](core::Result<core::NotebookOutline> outline) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, outline = std::move(outline)] mutable {
                showLoadedOutline(opening, std::move(outline));
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::showLoadedOutline(std::uint64_t opening,
                                          core::Result<core::NotebookOutline> outline) {
    if (opening != m_opening) {
        return;
    }
    if (!outline) {
        reportError(QString::fromStdString(outline.error().message));
        return;
    }
    m_outline = core::Outline{std::move(*outline)};
    publishOutline();
    for (const core::SectionInfo& section : m_outline.sections()) {
        for (const core::PageInfo& page : section.pages) {
            if (QString::fromStdString(page.id.toString()) == m_startPage) {
                goToPage(page.id);
                return;
            }
        }
    }
    if (!m_outline.sections().empty() && !m_outline.sections().front().pages.empty()) {
        goToPage(m_outline.sections().front().pages.front().id);
    }
}

void NotebookViewModel::goToPage(const core::Uuid& pageId) {
    m_erasing.clear();
    m_currentPage = pageId;
    publishOutline();
    emit currentPageChanged();
    emit pageStyleChanged();

    if (m_pages.contains(pageId)) {
        setLoaded(true);
        emit pageChanged();
        refreshCanvas();
        return;
    }

    setLoaded(false);
    emit pageChanged();
    refreshCanvas();
    if (!m_storage) {
        return;
    }
    const std::uint64_t opening = m_opening;
    m_storage->loadPage(
        pageId, [this, opening, pageId](core::Result<std::vector<core::PlacedStroke>> strokes) {
            QMetaObject::invokeMethod(
                this,
                [this, opening, pageId, strokes = std::move(strokes)] mutable {
                    showLoadedPage(opening, pageId, std::move(strokes));
                },
                Qt::QueuedConnection);
        });
}

void NotebookViewModel::showLoadedPage(std::uint64_t opening, const core::Uuid& pageId,
                                       core::Result<std::vector<core::PlacedStroke>> strokes) {
    if (opening != m_opening) {
        return;
    }
    if (!strokes) {
        reportError(QString::fromStdString(strokes.error().message));
        return;
    }
    if (!m_pages.contains(pageId)) {
        m_pages.emplace(pageId, std::make_unique<core::Page>(pageId, std::move(*strokes)));
    }
    if (pageId == m_currentPage) {
        setLoaded(true);
        emit pageChanged();
        refreshCanvas();
    }
}

void NotebookViewModel::goToPlace(std::size_t section, std::size_t page) {
    const std::span<const core::SectionInfo> sections = m_outline.sections();
    if (section >= sections.size() || page >= sections[section].pages.size()) {
        return;
    }
    goToPage(sections[section].pages[page].id);
}

void NotebookViewModel::setLoaded(bool loaded) {
    if (loaded != m_loaded) {
        m_loaded = loaded;
        emit loadedChanged();
    }
}

void NotebookViewModel::setCanvas(platform::ink::QtInkItem* canvas) {
    if (m_canvas == canvas) {
        return;
    }
    if (!m_canvas.isNull()) {
        m_canvas->setSink(nullptr);
    }
    m_canvas = canvas;
    emit canvasChanged();

    if (m_canvas.isNull()) {
        return;
    }
    m_canvas->setSink(&m_sink);
    refreshCanvas();
}

QString NotebookViewModel::title() const {
    return QString::fromStdString(m_outline.contents().title);
}

core::Page* NotebookViewModel::currentPageData() {
    const auto found = m_pages.find(m_currentPage);
    return found == m_pages.end() ? nullptr : found->second.get();
}

const core::Page* NotebookViewModel::currentPageData() const {
    const auto found = m_pages.find(m_currentPage);
    return found == m_pages.end() ? nullptr : found->second.get();
}

const core::PageInfo* NotebookViewModel::currentPageInfo() const {
    return m_outline.page(m_currentPage);
}

std::optional<core::PagePlace> NotebookViewModel::currentPlace() const {
    return m_outline.placeOf(m_currentPage);
}

std::optional<std::size_t> NotebookViewModel::currentSectionIndex() const {
    const std::optional<core::PagePlace> place = currentPlace();
    return place ? m_outline.sectionIndex(place->sectionId) : std::nullopt;
}

int NotebookViewModel::strokeCount() const {
    const core::Page* const page = currentPageData();
    return page == nullptr ? 0 : static_cast<int>(page->strokes().size());
}

int NotebookViewModel::currentSection() const {
    const std::optional<std::size_t> section = currentSectionIndex();
    return section ? static_cast<int>(*section) : -1;
}

void NotebookViewModel::setCurrentSection(int index) {
    if (const std::optional<std::size_t> section = checkedIndex(index, m_outline.sections().size());
        section && static_cast<int>(*section) != currentSection()) {
        goToPlace(*section, 0);
    }
}

int NotebookViewModel::currentPage() const {
    const std::optional<core::PagePlace> place = currentPlace();
    return place ? static_cast<int>(place->index) : -1;
}

void NotebookViewModel::setCurrentPage(int index) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section) {
        return;
    }
    if (const std::optional<std::size_t> page =
            checkedIndex(index, m_outline.sections()[*section].pages.size());
        page && static_cast<int>(*page) != currentPage()) {
        goToPlace(*section, *page);
    }
}

int NotebookViewModel::sectionCount() const {
    return static_cast<int>(m_outline.sections().size());
}

int NotebookViewModel::pageCount() const {
    const std::optional<std::size_t> section = currentSectionIndex();
    return section ? static_cast<int>(m_outline.sections()[*section].pages.size()) : 0;
}

bool NotebookViewModel::hasPreviousPage() const {
    return currentPage() > 0 || currentSection() > 0;
}

bool NotebookViewModel::hasNextPage() const {
    return (currentPage() >= 0 && currentPage() + 1 < pageCount())
           || (currentSection() >= 0 && currentSection() + 1 < sectionCount());
}

void NotebookViewModel::previousPage() {
    const std::optional<std::size_t> section = currentSectionIndex();
    const std::optional<core::PagePlace> place = currentPlace();
    if (!section || !place) {
        return;
    }
    if (place->index > 0) {
        goToPlace(*section, place->index - 1);
    } else if (*section > 0) {
        goToPlace(*section - 1, lastIndex(m_outline.sections()[*section - 1].pages.size()));
    }
}

void NotebookViewModel::nextPage() {
    const std::optional<std::size_t> section = currentSectionIndex();
    const std::optional<core::PagePlace> place = currentPlace();
    if (!section || !place) {
        return;
    }
    if (place->index + 1 < m_outline.sections()[*section].pages.size()) {
        goToPlace(*section, place->index + 1);
    } else if (*section + 1 < m_outline.sections().size()) {
        goToPlace(*section + 1, 0);
    }
}

page_options::Paper NotebookViewModel::paper() const {
    const core::PageInfo* const info = currentPageInfo();
    return static_cast<page_options::Paper>(info == nullptr ? core::PageStyle{}.paper
                                                            : info->style.paper);
}

void NotebookViewModel::setPaper(page_options::Paper paper) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.paper = static_cast<core::Paper>(paper);
        changeStyle(style);
    }
}

page_options::Orientation NotebookViewModel::orientation() const {
    const core::PageInfo* const info = currentPageInfo();
    return static_cast<page_options::Orientation>(info == nullptr ? core::PageStyle{}.orientation
                                                                  : info->style.orientation);
}

void NotebookViewModel::setOrientation(page_options::Orientation orientation) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.orientation = static_cast<core::Orientation>(orientation);
        changeStyle(style);
    }
}

page_options::Background NotebookViewModel::background() const {
    const core::PageInfo* const info = currentPageInfo();
    return static_cast<page_options::Background>(info == nullptr ? core::PageStyle{}.background
                                                                 : info->style.background);
}

void NotebookViewModel::setBackground(page_options::Background background) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.background = static_cast<core::Background>(background);
        changeStyle(style);
    }
}

qreal NotebookViewModel::lineSpacing() const {
    const core::PageInfo* const info = currentPageInfo();
    const float spacing = info == nullptr ? core::PageStyle{}.spacing : info->style.spacing;
    return spacing / core::millimeters(1.0F);
}

void NotebookViewModel::setLineSpacing(qreal millimeters) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.spacing = core::millimeters(static_cast<float>(millimeters));
        changeStyle(core::normalized(style));
    }
}

void NotebookViewModel::changeStyle(const core::PageStyle& style) {
    const core::PageInfo* const info = currentPageInfo();
    if (info == nullptr || info->style == style || !m_storage) {
        return;
    }
    runCommand(
        std::make_unique<core::SetPageStyleCommand>(&m_outline, &*m_storage, m_currentPage, style));
}

void NotebookViewModel::Sink::strokeStarted(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::sampleAdded(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::strokeFinished(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::strokeCompleted(const core::Stroke& stroke) {
    m_owner->storeStroke(stroke);
}

void NotebookViewModel::Sink::strokeCancelled() {}

void NotebookViewModel::Sink::eraserMoved(const core::InkSample& from, const core::InkSample& to,
                                          float radius) {
    m_owner->erase(from, to, radius);
}

void NotebookViewModel::Sink::eraseFinished() {
    m_owner->finishErasing();
}

void NotebookViewModel::storeStroke(const core::Stroke& stroke) {
    core::Page* const page = currentPageData();
    if (!m_loaded || page == nullptr || !m_storage) {
        refreshCanvas();
        return;
    }
    const core::Result<void> stored = m_history.run(std::make_unique<core::AddStrokeCommand>(
        page, &*m_storage, core::PlacedStroke{.ordinal = page->nextOrdinal(), .stroke = stroke}));
    if (!stored) {
        reportError(QString::fromStdString(stored.error().message));
        refreshCanvas();
        return;
    }
    emit pageChanged();
    emit historyChanged();
}

void NotebookViewModel::erase(const core::InkSample& from, const core::InkSample& to,
                              float radius) {
    const core::Page* const page = currentPageData();
    if (!m_loaded || page == nullptr) {
        return;
    }
    const core::EraserSweep sweep{
        .from = {.x = from.x, .y = from.y},
        .to = {.x = to.x, .y = to.y},
        .radius = radius,
    };
    bool found = false;
    for (const core::Uuid& strokeId : page->strokesTouchedBy(sweep)) {
        if (std::ranges::find(m_erasing, strokeId) == m_erasing.end()) {
            m_erasing.push_back(strokeId);
            found = true;
        }
    }
    if (found) {
        refreshCanvas();
    }
}

void NotebookViewModel::finishErasing() {
    core::Page* const page = currentPageData();
    if (m_erasing.empty() || page == nullptr || !m_storage) {
        m_erasing.clear();
        return;
    }
    runCommand(std::make_unique<core::EraseStrokesCommand>(page, &*m_storage,
                                                           std::exchange(m_erasing, {})));
}

void NotebookViewModel::undo() {
    m_erasing.clear();
    if (const core::ICommand* const next = m_history.nextUndo()) {
        const std::optional<core::Uuid> pageToShow = next->pageToShow();
        finishChange(m_history.undo(), pageToShow);
    }
}

void NotebookViewModel::redo() {
    m_erasing.clear();
    if (const core::ICommand* const next = m_history.nextRedo()) {
        const std::optional<core::Uuid> pageToShow = next->pageToShow();
        finishChange(m_history.redo(), pageToShow);
    }
}

void NotebookViewModel::clearPage() {
    m_erasing.clear();
    core::Page* const page = currentPageData();
    if (m_loaded && page != nullptr && m_storage) {
        runCommand(std::make_unique<core::ClearPageCommand>(page, &*m_storage));
    }
}

void NotebookViewModel::addPage() {
    const std::optional<core::PagePlace> place = currentPlace();
    const core::PageInfo* const info = currentPageInfo();
    if (!place || info == nullptr || !m_storage) {
        return;
    }
    const core::PageInfo page{.id = m_ids.next(), .title = {}, .style = info->style};
    m_pages.emplace(page.id, std::make_unique<core::Page>(page.id));
    runCommand(std::make_unique<core::AddPageCommand>(
        &m_outline, &*m_storage,
        core::PagePlace{.sectionId = place->sectionId, .index = place->index + 1}, page));
}

void NotebookViewModel::deletePage(int index) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    if (const std::optional<std::size_t> page = checkedIndex(index, pages.size())) {
        runCommand(
            std::make_unique<core::DeletePageCommand>(&m_outline, &*m_storage, pages[*page].id));
    }
}

void NotebookViewModel::movePage(int from, int to) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const core::SectionInfo& info = m_outline.sections()[*section];
    const std::optional<std::size_t> source = checkedIndex(from, info.pages.size());
    const std::optional<std::size_t> target = checkedIndex(to, info.pages.size());
    if (source && target && source != target) {
        runCommand(std::make_unique<core::MovePageCommand>(
            &m_outline, &*m_storage, info.pages[*source].id,
            core::PagePlace{.sectionId = info.id, .index = *target}));
    }
}

void NotebookViewModel::renamePage(int index, const QString& title) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    const std::string trimmed = title.trimmed().toStdString();
    if (const std::optional<std::size_t> page = checkedIndex(index, pages.size());
        page && pages[*page].title != trimmed) {
        runCommand(std::make_unique<core::RenamePageCommand>(&m_outline, &*m_storage,
                                                             pages[*page].id, trimmed));
    }
}

void NotebookViewModel::addSection() {
    if (!m_storage) {
        return;
    }
    const core::PageInfo* const info = currentPageInfo();
    const core::PageInfo page{
        .id = m_ids.next(),
        .title = {},
        .style = info == nullptr ? core::PageStyle{} : info->style,
    };
    const std::size_t index = m_outline.sections().size();
    core::SectionInfo section{
        .id = m_ids.next(),
        .title = tr("Section %1").arg(index + 1).toStdString(),
        .pages = {page},
    };
    m_pages.emplace(page.id, std::make_unique<core::Page>(page.id));
    runCommand(std::make_unique<core::AddSectionCommand>(&m_outline, &*m_storage, index,
                                                         std::move(section)));
}

void NotebookViewModel::deleteSection(int index) {
    const std::span<const core::SectionInfo> sections = m_outline.sections();
    if (const std::optional<std::size_t> section = checkedIndex(index, sections.size());
        section && m_storage) {
        runCommand(std::make_unique<core::DeleteSectionCommand>(&m_outline, &*m_storage,
                                                                sections[*section].id));
    }
}

void NotebookViewModel::moveSection(int from, int to) {
    const std::span<const core::SectionInfo> sections = m_outline.sections();
    const std::optional<std::size_t> source = checkedIndex(from, sections.size());
    const std::optional<std::size_t> target = checkedIndex(to, sections.size());
    if (source && target && source != target && m_storage) {
        runCommand(std::make_unique<core::MoveSectionCommand>(&m_outline, &*m_storage,
                                                              sections[*source].id, *target));
    }
}

void NotebookViewModel::renameSection(int index, const QString& title) {
    const std::span<const core::SectionInfo> sections = m_outline.sections();
    const std::string trimmed = title.trimmed().toStdString();
    if (const std::optional<std::size_t> section = checkedIndex(index, sections.size());
        section && m_storage && !trimmed.empty() && sections[*section].title != trimmed) {
        runCommand(std::make_unique<core::RenameSectionCommand>(&m_outline, &*m_storage,
                                                                sections[*section].id, trimmed));
    }
}

void NotebookViewModel::runCommand(std::unique_ptr<core::ICommand> command) {
    const std::optional<core::Uuid> pageToShow = command->pageToShow();
    finishChange(m_history.run(std::move(command)), pageToShow);
}

void NotebookViewModel::finishChange(const core::Result<void>& change,
                                     std::optional<core::Uuid> pageToShow) {
    if (!change) {
        reportError(QString::fromStdString(change.error().message));
    }
    emit historyChanged();

    if (pageToShow && *pageToShow != m_currentPage && m_outline.page(*pageToShow) != nullptr) {
        goToPage(*pageToShow);
        return;
    }
    if (m_outline.page(m_currentPage) == nullptr) {
        const std::span<const core::SectionInfo> sections = m_outline.sections();
        if (!sections.empty()) {
            const auto section = static_cast<std::size_t>(
                std::clamp(currentSection(), 0, static_cast<int>(lastIndex(sections.size()))));
            publishOutline();
            goToPlace(section, 0);
            return;
        }
    }
    publishOutline();
    emit currentPageChanged();
    emit pageStyleChanged();
    emit pageChanged();
    refreshCanvas();
}

void NotebookViewModel::publishOutline() {
    std::vector<OutlineItem> sections;
    sections.reserve(m_outline.sections().size());
    for (const core::SectionInfo& section : m_outline.sections()) {
        sections.push_back(OutlineItem{
            .title = QString::fromStdString(section.title),
            .count = static_cast<int>(section.pages.size()),
        });
    }

    std::vector<OutlineItem> pages;
    if (const std::optional<std::size_t> section = currentSectionIndex()) {
        const std::vector<core::PageInfo>& infos = m_outline.sections()[*section].pages;
        pages.reserve(infos.size());
        for (std::size_t i = 0; i < infos.size(); ++i) {
            pages.push_back(OutlineItem{
                .title = infos[i].title.empty() ? tr("Page %1").arg(i + 1)
                                                : QString::fromStdString(infos[i].title),
                .count = 0,
            });
        }
    }

    m_sectionsModel.setItems(std::move(sections));
    m_pagesModel.setItems(std::move(pages));
    emit outlineChanged();
}

void NotebookViewModel::refreshCanvas() {
    if (m_canvas.isNull()) {
        return;
    }
    const core::PageInfo* const info = currentPageInfo();
    const core::PageStyle style = info == nullptr ? core::PageStyle{} : info->style;
    if (const core::Page* const page = currentPageData()) {
        m_canvas->showPage(*page, style, m_erasing);
        return;
    }
    const core::Page placeholder{m_currentPage};
    m_canvas->showPage(placeholder, style);
}

void NotebookViewModel::reportError(const QString& message) {
    qWarning("%s", qUtf8Printable(message));
    m_errorMessage = message;
    emit errorMessageChanged();
}

}
