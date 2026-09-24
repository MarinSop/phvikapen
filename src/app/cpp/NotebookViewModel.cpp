#include "app/cpp/NotebookViewModel.hpp"

#include "app/cpp/HandwritingReader.hpp"
#include "app/cpp/OutlineModels.hpp"
#include "app/cpp/Thumbnails.hpp"
#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/StrokeEraser.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/ink/StrokeSelection.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/text/InkWord.hpp"
#include "core/text/WrittenText.hpp"
#include "core/undo/BundleCommand.hpp"
#include "core/undo/OutlineCommands.hpp"
#include "core/undo/StrokeCommands.hpp"
#include "core/undo/TextCommands.hpp"
#include "platform/pdf/PdfRenderer.hpp"
#include "platform/render/PagePainter.hpp"
#include "platform/render/PaperLook.hpp"
#include "platform/render/PdfExporter.hpp"

#include <QClipboard>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMetaObject>
#include <QPainter>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QtLogging>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <numbers>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr auto kDefaultNotebookName = "default.phvika";
// More than a reader would ever look through at once.
constexpr std::size_t kMostFound = 300;
constexpr auto kKeptPrefix = "kept/";
constexpr int kMaximumMediaPixels = 4096;
constexpr qreal kMediaRedrawFactor = 1.4;
constexpr int kMediaRedrawDelay = 200;
constexpr float kPasteOffset = 24.0F;
constexpr float kPickRadius = 6.0F;
constexpr qreal kTextMargin = 8.0;
constexpr qreal kNewTextWidth = core::TextBox::kDefaultWidth;
constexpr float kOwnPaperWidth = core::millimeters(210.0F);
constexpr float kOwnPaperHeight = core::millimeters(297.0F);
constexpr int kPagesAround = 3;
constexpr int kMediaAround = 5;

// The picture of a page belongs to the sheet it was drawn for; a sheet of another size needs
// another picture.
[[nodiscard]] bool fitsSheet(const QRectF& area, const core::PaperSize& paper) {
    const auto wide = static_cast<qreal>(paper.width);
    const auto tall = static_cast<qreal>(paper.height);
    return qFuzzyCompare(area.width() + 1.0, wide + 1.0)
           && qFuzzyCompare(area.height() + 1.0, tall + 1.0);
}

constexpr float kEmptyPageRatio = std::numbers::sqrt2_v<float>;
constexpr qreal kSmallestMediaScale = 0.5;
constexpr float kColumnMediaScale = 2.0F;
constexpr double kMostMediaPixels = 8e6;
constexpr qreal kFineEnough = 0.01;

[[nodiscard]] core::ContentId hashOf(const QByteArray& data) {
    const QByteArray digest = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    core::ContentId::Bytes bytes{};
    if (static_cast<std::size_t>(digest.size()) == bytes.size()) {
        std::ranges::transform(digest, bytes.begin(),
                               [](char value) { return static_cast<std::uint8_t>(value); });
    }
    return core::ContentId{bytes};
}

[[nodiscard]] std::vector<std::byte> toBytes(const QByteArray& data) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(data.size()));
    std::ranges::transform(data, bytes.begin(),
                           [](char value) { return static_cast<std::byte>(value); });
    return bytes;
}

[[nodiscard]] QByteArray toByteArray(const std::vector<std::byte>& bytes) {
    QByteArray data(static_cast<qsizetype>(bytes.size()), Qt::Uninitialized);
    std::ranges::transform(bytes, data.begin(),
                           [](std::byte value) { return static_cast<char>(value); });
    return data;
}

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

NotebookViewModel::NotebookViewModel(QObject* parent) : QObject(parent) {
    m_mediaTimer.setSingleShot(true);
    m_mediaTimer.setInterval(kMediaRedrawDelay);
    connect(&m_mediaTimer, &QTimer::timeout, this, &NotebookViewModel::redrawMedia);
    connect(&m_reader, &HandwritingReader::pagesWaitingChanged, this, [this](int waiting) {
        if (waiting == m_pagesToRead) {
            return;
        }
        m_pagesToRead = waiting;
        emit readingChanged();
    });
    connect(&m_reader, &HandwritingReader::failed, this, &NotebookViewModel::reportError);
}

NotebookViewModel::NotebookViewModel(QString path, QString startPage, QObject* parent)
    : NotebookViewModel(parent) {
    m_notebookPath = std::move(path);
    m_startPage = std::move(startPage);
    m_completed = true;
    openNotebook();
}

QString NotebookViewModel::name() const {
    return QFileInfo{m_notebookPath}.completeBaseName();
}

void NotebookViewModel::readKeptAt() {
    const QSettings settings;
    const QString kept = settings.value(QString{kKeptPrefix} + name()).toString();
    if (kept == m_keptAt) {
        return;
    }
    m_keptAt = kept;
    emit keptAtChanged();
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
        std::filesystem::path fromSide = from;
        std::filesystem::path toSide = to;
        fromSide += suffix;
        toSide += suffix;
        std::error_code ignored;
        std::filesystem::rename(fromSide, toSide, ignored);
    }

    QSettings settings;
    QString wasKept = settings.value(QString{kKeptPrefix} + name()).toString();
    settings.remove(QString{kKeptPrefix} + name());
    m_notebookPath = path;
    // The file the reader keeps goes by the notebook's name as well.
    if (!wasKept.isEmpty() && QFile::exists(wasKept)) {
        const QFileInfo kept{wasKept};
        const QString renamed = kept.absolutePath() + "/" + name() + "." + kept.suffix();
        if (renamed == wasKept || QFile::rename(wasKept, renamed)) {
            wasKept = renamed;
        }
    }
    if (!wasKept.isEmpty()) {
        m_keptAt = wasKept;
        emit keptAtChanged();
        settings.setValue(QString{kKeptPrefix} + name(), wasKept);
    }
    emit notebookPathChanged();
    openNotebook();
    return true;
}

NotebookViewModel::~NotebookViewModel() {
    if (m_export.joinable()) {
        m_export.join();
    }
    if (m_pictures.joinable()) {
        m_pictures.join();
    }
    if (!m_canvas.isNull()) {
        m_canvas->setSink(nullptr);
    }
}

void NotebookViewModel::applyStyle(const core::PageStyle& style) {
    changeStyle(style);
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
    m_shownMedia.clear();
    m_drawnAt.clear();
    m_wantedMedia.clear();
    m_drawing.clear();
    m_documentSizes.clear();
    m_outline = core::Outline{};
    m_currentPage = core::Uuid{};
    const std::uint64_t opening = ++m_opening;
    setLoaded(false);
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit errorMessageChanged();
    }
    publishOutline();
    readKeptAt();
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
    m_reader.open(path);
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
    if (!m_canvas.isNull() && !m_currentPage.isNil() && m_currentPage != pageId) {
        m_views.insert_or_assign(m_currentPage, m_canvas->viewport());
    }
    m_erasing.clear();
    m_currentPage = pageId;
    publishOutline();
    emit currentPageChanged();
    emit pageStyleChanged();

    if (m_pages.contains(pageId)) {
        setLoaded(true);
        emit pageChanged();
        refreshCanvas();
        refreshMedia();
        return;
    }

    setLoaded(false);
    emit pageChanged();
    refreshCanvas();
    refreshMedia();
    if (!m_storage) {
        return;
    }
    const std::uint64_t opening = m_opening;
    m_storage->loadPage(pageId, [this, opening, pageId](core::Result<core::LoadedPage> loaded) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, pageId, loaded = std::move(loaded)] mutable {
                showLoadedPage(opening, pageId, std::move(loaded));
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::showLoadedPage(std::uint64_t opening, const core::Uuid& pageId,
                                       core::Result<core::LoadedPage> loaded) {
    if (opening != m_opening) {
        return;
    }
    if (!loaded) {
        reportError(QString::fromStdString(loaded.error().message));
        return;
    }
    if (!m_pages.contains(pageId)) {
        m_pages.emplace(pageId, std::make_unique<core::Page>(pageId, std::move(loaded->strokes),
                                                             std::move(loaded->texts)));
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
    connect(m_canvas, &platform::ink::QtInkItem::viewChanged, this, [this] {
        if (m_canvas.isNull()) {
            return;
        }
        if (m_canvas->zoom() > m_mediaScale * kMediaRedrawFactor) {
            m_mediaTimer.start();
        }
    });
    connect(m_canvas, &platform::ink::QtInkItem::pageWanted, this,
            &NotebookViewModel::goToShownPage);
    refreshCanvas();
    refreshMedia();
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
    const core::PageInfo* const info = currentPageInfo();
    if (info == nullptr) {
        return;
    }
    core::PageStyle style = info->style;
    style.paper = static_cast<core::Paper>(paper);
    if (style.paper == core::Paper::Custom && !core::paperSize(style)) {
        style.customWidth = kOwnPaperWidth;
        style.customHeight = kOwnPaperHeight;
    }
    changeStyle(style);
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

namespace {

[[nodiscard]] QColor asQColor(core::Color color) {
    return color.alpha == 0 ? QColor{}
                            : QColor::fromRgb(color.red, color.green, color.blue, color.alpha);
}

[[nodiscard]] core::Color asColor(const QColor& color) {
    if (!color.isValid()) {
        return core::PageStyle::kUnset;
    }
    return core::Color{
        .red = static_cast<std::uint8_t>(color.red()),
        .green = static_cast<std::uint8_t>(color.green()),
        .blue = static_cast<std::uint8_t>(color.blue()),
        .alpha = static_cast<std::uint8_t>(color.alpha()),
    };
}

}

// The look of the paper: a colour nobody chose comes back as an invalid one, which the window
// shows as "the usual one for this ruling".
QColor NotebookViewModel::paperColor() const {
    const core::PageInfo* const info = currentPageInfo();
    return asQColor(info == nullptr ? core::PageStyle{}.paperColor : info->style.paperColor);
}

void NotebookViewModel::setPaperColor(const QColor& color) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.paperColor = asColor(color);
        changeStyle(core::normalized(style));
    }
}

QColor NotebookViewModel::lineColor() const {
    const core::PageInfo* const info = currentPageInfo();
    return asQColor(info == nullptr ? core::PageStyle{}.lineColor : info->style.lineColor);
}

void NotebookViewModel::setLineColor(const QColor& color) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.lineColor = asColor(color);
        changeStyle(core::normalized(style));
    }
}

QColor NotebookViewModel::marginColor() const {
    const core::PageInfo* const info = currentPageInfo();
    return asQColor(info == nullptr ? core::PageStyle{}.marginColor : info->style.marginColor);
}

void NotebookViewModel::setMarginColor(const QColor& color) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.marginColor = asColor(color);
        changeStyle(core::normalized(style));
    }
}

qreal NotebookViewModel::lineWidth() const {
    const core::PageInfo* const info = currentPageInfo();
    return info == nullptr ? core::PageStyle{}.lineWidth : info->style.lineWidth;
}

void NotebookViewModel::setLineWidth(qreal width) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.lineWidth = static_cast<float>(width);
        changeStyle(core::normalized(style));
    }
}

qreal NotebookViewModel::marginAt() const {
    const core::PageInfo* const info = currentPageInfo();
    const float at = info == nullptr ? core::PageStyle{}.marginAt : info->style.marginAt;
    return at / core::millimeters(1.0F);
}

void NotebookViewModel::setMarginAt(qreal millimeters) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.marginAt = core::millimeters(static_cast<float>(millimeters));
        changeStyle(core::normalized(style));
    }
}

bool NotebookViewModel::margin() const {
    const core::PageInfo* const info = currentPageInfo();
    return info == nullptr ? core::PageStyle{}.margin : info->style.margin;
}

void NotebookViewModel::setMargin(bool shown) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.margin = shown;
        changeStyle(core::normalized(style));
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

qreal NotebookViewModel::customWidth() const {
    const core::PageInfo* const info = currentPageInfo();
    const float width = info == nullptr ? 0.0F : info->style.customWidth;
    return width / core::millimeters(1.0F);
}

void NotebookViewModel::setCustomWidth(qreal millimeters) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.customWidth = core::millimeters(static_cast<float>(millimeters));
        changeStyle(core::normalized(style));
    }
}

qreal NotebookViewModel::customHeight() const {
    const core::PageInfo* const info = currentPageInfo();
    const float height = info == nullptr ? 0.0F : info->style.customHeight;
    return height / core::millimeters(1.0F);
}

void NotebookViewModel::setCustomHeight(qreal millimeters) {
    if (const core::PageInfo* const info = currentPageInfo()) {
        core::PageStyle style = info->style;
        style.customHeight = core::millimeters(static_cast<float>(millimeters));
        changeStyle(core::normalized(style));
    }
}

void NotebookViewModel::applyStyleToSection() {
    if (const core::PageInfo* const info = currentPageInfo()) {
        changeStyle(info->style);
    }
}

// The setup belongs to the section: every page in it is written on the same paper.
void NotebookViewModel::changeStyle(const core::PageStyle& style) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    std::vector<core::Uuid> wanted;
    for (const core::PageInfo& page : m_outline.sections()[*section].pages) {
        if (page.style != style) {
            wanted.push_back(page.id);
        }
    }
    if (wanted.empty()) {
        return;
    }
    // The small pictures were drawn on the old paper, so they are thrown away with it.
    for (const core::Uuid& page : wanted) {
        forgetThumbnail(page);
    }
    if (!m_storage) {
        return;
    }
    runCommand(std::make_unique<core::SetSectionStyleCommand>(&m_outline, &*m_storage,
                                                              std::move(wanted), style));
}

// The setup of a single page. Nothing asks for it now that the section carries the paper, but
// pages may be given their own again.
void NotebookViewModel::changeStyleOfPage(const core::PageStyle& style) {
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

void NotebookViewModel::Sink::colourWanted(const core::InkSample& at, int sheet) {
    m_owner->pickColour(at, sheet);
}

void NotebookViewModel::Sink::strokeCompleted(const core::Stroke& stroke, int sheet) {
    m_owner->storeStroke(stroke, sheet);
}

void NotebookViewModel::Sink::strokeCancelled() {}

void NotebookViewModel::Sink::eraserMoved(const core::InkSample& from, const core::InkSample& to,
                                          float radius, int sheet) {
    m_owner->erase(from, to, radius, sheet);
}

void NotebookViewModel::Sink::selectionDrawn(std::span<const core::Point> shape) {
    m_owner->selectInside(shape);
}

void NotebookViewModel::Sink::selectionMoved(float dx, float dy) {
    m_owner->moveSelection(dx, dy);
}

void NotebookViewModel::Sink::eraseFinished() {
    m_owner->finishErasing();
}

core::Uuid NotebookViewModel::pageOfSheet(int sheet) const {
    const std::optional<std::size_t> section = currentSectionIndex();
    // Only a column has sheets to tell apart; on its own, a page is the one being read.
    if (!m_continuous || sheet < 0 || !section) {
        return m_currentPage;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    const auto at = static_cast<std::size_t>(sheet);
    return at < pages.size() ? pages[at].id : m_currentPage;
}

void NotebookViewModel::storeStroke(const core::Stroke& stroke, int sheet) {
    const core::Uuid on = pageOfSheet(sheet);
    forgetThumbnail(on);
    const auto found = m_pages.find(on);
    core::Page* const page = found == m_pages.end() ? nullptr : found->second.get();
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
    markEdited();
    emit historyChanged();
}

void NotebookViewModel::pickColour(const core::InkSample& at, int sheet) {
    const auto found = m_pages.find(pageOfSheet(sheet));
    const core::Page* const page = found == m_pages.end() ? nullptr : found->second.get();
    if (page == nullptr) {
        return;
    }
    const std::span<const core::PlacedStroke> strokes = page->strokes();
    for (const core::PlacedStroke& placed : std::ranges::reverse_view(strokes)) {
        const core::EraserSweep spot{
            .from = {.x = at.x, .y = at.y},
            .to = {.x = at.x, .y = at.y},
            .radius = std::max(placed.stroke.style().width / 2.0F, kPickRadius),
        };
        if (core::touches(placed.stroke, spot)) {
            const core::Color& colour = placed.stroke.style().color;
            emit colourPicked(QColor::fromRgb(colour.red, colour.green, colour.blue, colour.alpha));
            return;
        }
    }
    pickFromMedia(at);
}

// Where no line was drawn, the colour comes from the document underneath.
void NotebookViewModel::pickFromMedia(const core::InkSample& at) {
    const auto piece = m_shownMedia.find(m_currentPage);
    if (piece == m_shownMedia.end() || piece->second.picture.isNull()) {
        return;
    }
    const QRectF& area = piece->second.area;
    const QImage& picture = piece->second.picture;
    if (area.isEmpty() || !area.contains(at.x, at.y)) {
        return;
    }
    const auto column = static_cast<int>((at.x - area.left()) / area.width() * picture.width());
    const auto row = static_cast<int>((at.y - area.top()) / area.height() * picture.height());
    const QColor colour = picture.pixelColor(std::clamp(column, 0, picture.width() - 1),
                                             std::clamp(row, 0, picture.height() - 1));
    if (colour.alpha() > 0) {
        emit colourPicked(QColor::fromRgb(colour.red(), colour.green(), colour.blue()));
    }
}

void NotebookViewModel::erase(const core::InkSample& from, const core::InkSample& to, float radius,
                              int sheet) {
    const core::Uuid on = pageOfSheet(sheet);
    const auto kept = m_pages.find(on);
    const core::Page* const page = kept == m_pages.end() ? nullptr : kept->second.get();
    if (!m_loaded || page == nullptr) {
        return;
    }
    // A sweep stays on the page it started on, even where the eraser runs past its edge.
    m_erasedPage = on;
    const core::EraserSweep sweep{
        .from = {.x = from.x, .y = from.y},
        .to = {.x = to.x, .y = to.y},
        .radius = radius,
    };
    m_sweeps.push_back(sweep);

    bool changed = false;
    for (const core::Uuid& strokeId : page->strokesTouchedBy(sweep)) {
        if (!m_erasePieces.contains(strokeId)) {
            const core::PlacedStroke* found = nullptr;
            for (const core::PlacedStroke& placed : page->strokes()) {
                if (placed.stroke.id() == strokeId) {
                    found = &placed;
                    break;
                }
            }
            if (found == nullptr) {
                continue;
            }
            m_erasePieces.emplace(strokeId, std::vector<core::Stroke>{found->stroke});
            m_erasing.push_back(strokeId);
        }
    }

    const std::span<const core::EraserSweep> latest{&m_sweeps.back(), 1};
    for (auto& [strokeId, pieces] : m_erasePieces) {
        std::vector<core::Stroke> left;
        for (const core::Stroke& piece : pieces) {
            std::vector<core::Stroke> cut = core::erased(piece, latest, m_ids);
            if (core::wholeStrokeSurvives(piece, cut)) {
                left.push_back(piece);
                continue;
            }
            changed = true;
            left.insert(left.end(), std::make_move_iterator(cut.begin()),
                        std::make_move_iterator(cut.end()));
        }
        pieces = std::move(left);
    }

    if (changed) {
        refreshCanvas();
    }
}

void NotebookViewModel::finishErasing() {
    const auto kept = m_pages.find(m_erasedPage.isNil() ? m_currentPage : m_erasedPage);
    core::Page* const page = kept == m_pages.end() ? nullptr : kept->second.get();
    m_erasedPage = core::Uuid{};
    if (m_erasing.empty() || page == nullptr || !m_storage) {
        m_erasing.clear();
        m_erasePieces.clear();
        m_sweeps.clear();
        return;
    }

    std::int64_t ordinal = page->nextOrdinal();
    std::vector<core::PlacedStroke> pieces;
    for (const auto& [strokeId, left] : m_erasePieces) {
        for (const core::Stroke& piece : left) {
            pieces.push_back(core::PlacedStroke{.ordinal = ordinal, .stroke = piece});
            ++ordinal;
        }
    }

    std::vector<core::Uuid> erasedIds = std::exchange(m_erasing, {});
    m_erasePieces.clear();
    m_sweeps.clear();
    runCommand(std::make_unique<core::SplitStrokesCommand>(page, &*m_storage, std::move(erasedIds),
                                                           std::move(pieces)));
}

void NotebookViewModel::undo() {
    m_erasing.clear();
    m_erasePieces.clear();
    m_sweeps.clear();
    if (const core::ICommand* const next = m_history.nextUndo()) {
        const std::optional<core::Uuid> pageToShow = next->pageToShow();
        finishChange(m_history.undo(), pageToShow);
    }
}

void NotebookViewModel::redo() {
    m_erasing.clear();
    m_erasePieces.clear();
    m_sweeps.clear();
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

// "Page 3" is the name of a page, not of the third place in the section, so a new page takes the
// lowest number no other page goes by.
QString NotebookViewModel::freePageName(std::size_t section) const {
    const std::vector<core::PageInfo>& pages = m_outline.sections()[section].pages;
    for (std::size_t number = 1;; ++number) {
        const QString wanted = tr("Page %1").arg(number);
        const bool taken = std::ranges::any_of(pages, [&wanted](const core::PageInfo& page) {
            return QString::fromStdString(page.title) == wanted;
        });
        if (!taken) {
            return wanted;
        }
    }
}

// Pages made before names were given keep the names they are shown by, so that carrying one
// somewhere else does not renumber the rest. This is bookkeeping, not an edit, so it is not
// something to undo.
void NotebookViewModel::nameEveryPage(std::size_t section) {
    if (!m_storage) {
        return;
    }
    const std::vector<core::PageInfo> pages = m_outline.sections()[section].pages;
    for (std::size_t index = 0; index < pages.size(); ++index) {
        if (!pages[index].title.empty()) {
            continue;
        }
        const core::Uuid id = pages[index].id;
        // Held by a pointer the writer can take along without any chance of failing.
        const auto title =
            std::make_shared<const std::string>(tr("Page %1").arg(index + 1).toStdString());
        if (!m_outline.renamePage(id, *title)) {
            continue;
        }
        m_storage->submit(
            [id, title](core::NotebookStore& store) { return store.renamePage(id, *title); });
    }
}

void NotebookViewModel::addPage() {
    const std::optional<core::PagePlace> place = currentPlace();
    const std::optional<std::size_t> section = currentSectionIndex();
    const core::PageInfo* const info = currentPageInfo();
    if (!place || !section || info == nullptr || !m_storage) {
        return;
    }
    // Pages that still go by where they sit are named first, so the new one takes a free number.
    const std::size_t at = *section;
    const core::Uuid sectionId = place->sectionId;
    const std::size_t after = place->index;
    nameEveryPage(at);
    if (!m_storage) {
        return;
    }
    const core::PageInfo page{
        .id = m_ids.next(),
        // The name belongs to the page, not to the place it sits in, so it is given now.
        .title = freePageName(at).toStdString(),
        .style = info->style,
        .media = std::nullopt,
    };
    m_pages.emplace(page.id, std::make_unique<core::Page>(page.id));
    runCommand(std::make_unique<core::AddPageCommand>(
        &m_outline, &*m_storage, core::PagePlace{.sectionId = sectionId, .index = after + 1},
        page));
    emit pageAdded(currentPage());
}

void NotebookViewModel::duplicatePage(int index) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    const std::optional<std::size_t> at = checkedIndex(index, pages.size());
    if (!at) {
        return;
    }
    const core::PageInfo original = pages[*at];

    if (const auto cached = m_pages.find(original.id); cached != m_pages.end()) {
        copyPage(original, cached->second->strokes(), cached->second->texts());
        return;
    }

    const std::uint64_t opening = m_opening;
    const auto wanted = std::make_shared<const core::PageInfo>(original);
    m_storage->loadPage(wanted->id, [this, opening, wanted](core::Result<core::LoadedPage> loaded) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, wanted, loaded = std::move(loaded)] {
                if (opening != m_opening || !loaded) {
                    return;
                }
                copyPage(*wanted, loaded->strokes, loaded->texts);
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::copyPage(const core::PageInfo& original,
                                 std::span<const core::PlacedStroke> strokes,
                                 std::span<const core::PlacedText> texts) {
    const std::optional<core::PagePlace> place = m_outline.placeOf(original.id);
    if (!place || !m_storage) {
        return;
    }

    core::PageInfo copy{
        .id = m_ids.next(),
        .title = original.title,
        .style = original.style,
        .media = original.media,
    };

    std::vector<core::PlacedStroke> copies;
    copies.reserve(strokes.size());
    auto page = std::make_unique<core::Page>(copy.id);
    for (const core::PlacedStroke& placed : strokes) {
        core::Stroke fresh{m_ids.next(), placed.stroke.style()};
        for (const core::InkSample& sample : placed.stroke.samples()) {
            fresh.append(sample);
        }
        core::PlacedStroke made{.ordinal = placed.ordinal, .stroke = std::move(fresh)};
        std::ignore = page->insert(made);
        copies.push_back(std::move(made));
    }

    std::vector<core::PlacedText> textCopies;
    textCopies.reserve(texts.size());
    for (const core::PlacedText& placed : texts) {
        core::PlacedText made{.ordinal = placed.ordinal, .box = placed.box};
        made.box.id = m_ids.next();
        std::ignore = page->insertText(made);
        textCopies.push_back(std::move(made));
    }
    m_pages.insert_or_assign(copy.id, std::move(page));

    runCommand(std::make_unique<core::DuplicatePageCommand>(
        &m_outline, &*m_storage,
        core::PagePlace{.sectionId = place->sectionId, .index = place->index + 1}, std::move(copy),
        std::move(copies), std::move(textCopies)));
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
    const std::size_t at = *section;
    nameEveryPage(at);
    if (!m_storage) {
        return;
    }
    const core::SectionInfo& info = m_outline.sections()[at];
    const std::optional<std::size_t> source = checkedIndex(from, info.pages.size());
    const std::optional<std::size_t> target = checkedIndex(to, info.pages.size());
    if (source && target && source != target) {
        const std::size_t take = *source;
        const std::size_t put = *target;
        runCommand(std::make_unique<core::MovePageCommand>(
            &m_outline, &*m_storage, info.pages[take].id,
            core::PagePlace{.sectionId = info.id, .index = put}));
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
    const core::PageStyle defaultStyle;
    const core::PageInfo* const info = currentPageInfo();
    const core::PageInfo page{
        .id = m_ids.next(),
        .title = {},
        .style = info == nullptr ? defaultStyle : info->style,
        .media = std::nullopt,
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
    emit sectionAdded(currentSection());
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
    forgetThumbnail(pageToShow ? *pageToShow : m_currentPage);
    markEdited();
    m_reader.nudge();
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
    refreshMedia();
}

void NotebookViewModel::wantThumbnail(int index) {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    const std::optional<std::size_t> at = checkedIndex(index, pages.size());
    if (!at || m_thumbnails.contains(pages[*at].id)) {
        return;
    }

    const auto work = std::make_shared<ThumbnailWork>(ThumbnailWork{
        .page = pages[*at],
        .strokes = {},
        .texts = {},
        .media = {},
    });
    if (const auto cached = m_pages.find(work->page.id); cached != m_pages.end()) {
        const std::span<const core::PlacedStroke> strokes = cached->second->strokes();
        work->strokes.assign(strokes.begin(), strokes.end());
        const std::span<const core::PlacedText> texts = cached->second->texts();
        work->texts.assign(texts.begin(), texts.end());
        gatherThumbnail(work);
        return;
    }

    const std::uint64_t opening = m_opening;
    m_storage->loadPage(work->page.id,
                        [this, opening, work](core::Result<core::LoadedPage> loaded) {
                            QMetaObject::invokeMethod(
                                this,
                                [this, opening, work, loaded = std::move(loaded)] mutable {
                                    if (opening != m_opening || !loaded) {
                                        return;
                                    }
                                    work->strokes = std::move(loaded->strokes);
                                    work->texts = std::move(loaded->texts);
                                    gatherThumbnail(work);
                                },
                                Qt::QueuedConnection);
                        });
}

void NotebookViewModel::gatherThumbnail(const std::shared_ptr<ThumbnailWork>& work) {
    if (!work->page.media || !m_storage) {
        paintThumbnail(*work);
        return;
    }

    const std::uint64_t opening = m_opening;
    m_storage->loadAsset(work->page.media->asset,
                         [this, opening, work](core::Result<core::Asset> asset) {
                             QMetaObject::invokeMethod(
                                 this,
                                 [this, opening, work, asset = std::move(asset)] mutable {
                                     if (opening != m_opening) {
                                         return;
                                     }
                                     if (!asset) {
                                         paintThumbnail(*work);
                                         return;
                                     }
                                     thumbnailAsset(work, std::move(*asset));
                                 },
                                 Qt::QueuedConnection);
                         });
}

void NotebookViewModel::thumbnailAsset(const std::shared_ptr<ThumbnailWork>& work,
                                       core::Asset asset) {
    if (asset.kind == core::AssetKind::Image) {
        QImage picture;
        if (picture.loadFromData(toByteArray(asset.data))) {
            work->media = std::move(picture);
        }
        paintThumbnail(*work);
        return;
    }

    if (!m_pdf) {
        m_pdf.emplace();
    }
    const core::ContentId id = asset.id;
    const int index = work->page.media ? work->page.media->index : 0;
    const std::uint64_t opening = m_opening;
    m_pdf->open(std::move(asset), [](core::Result<std::vector<platform::pdf::PageSize>>) {});
    m_pdf->render(id, index, thumbnails::kWidth, thumbnails::kWidth * 2,
                  [this, opening, work](core::Result<platform::pdf::PageImage> image) {
                      if (!image) {
                          return;
                      }
                      QMetaObject::invokeMethod(
                          this,
                          [this, opening, work, drawn = std::move(*image)] {
                              if (opening == m_opening) {
                                  thumbnailPage(work, drawn);
                              }
                          },
                          Qt::QueuedConnection);
                  });
}

void NotebookViewModel::thumbnailPage(const std::shared_ptr<ThumbnailWork>& work,
                                      const platform::pdf::PageImage& image) {
    const QImage drawn{image.pixels.data(), image.width, image.height,
                       static_cast<qsizetype>(image.width) * 4, QImage::Format_RGBA8888};
    work->media = drawn.copy();
    paintThumbnail(*work);
}

void NotebookViewModel::paintThumbnail(const ThumbnailWork& work) {
    const platform::render::PageContents contents{
        .style = work.page.style,
        .strokes = work.strokes,
        .texts = work.texts,
        .media = work.media.isNull() ? nullptr : &work.media,
    };
    const core::Rect area = platform::render::pageArea(contents);
    const bool anything = area.width() > 0.0F && area.height() > 0.0F;
    // A page with nothing on it is still a page: it shows as the empty sheet it is.
    const float ratio = anything ? area.height() / area.width() : kEmptyPageRatio;
    const int height =
        std::max(1, static_cast<int>(static_cast<float>(thumbnails::kWidth) * ratio));
    QImage picture{thumbnails::kWidth, height, QImage::Format_ARGB32_Premultiplied};
    const platform::render::Rgba paper = platform::render::paperColorOf(work.page.style);
    picture.fill(QColor::fromRgbF(paper[0], paper[1], paper[2], paper[3]));
    if (anything) {
        QPainter painter{&picture};
        painter.scale(static_cast<double>(thumbnails::kWidth) / static_cast<double>(area.width()),
                      static_cast<double>(height) / static_cast<double>(area.height()));
        platform::render::paintPage(painter, contents, area);
    }

    const int revision = ++m_thumbnailRevision;
    m_thumbnails[work.page.id] = revision;
    thumbnails::put(
        QStringLiteral("%1-%2").arg(QString::fromStdString(work.page.id.toString())).arg(revision),
        picture);
    publishOutline();
}

void NotebookViewModel::forgetThumbnail(const core::Uuid& pageId) {
    if (m_thumbnails.erase(pageId) == 0) {
        return;
    }
    thumbnails::forget(QString::fromStdString(pageId.toString()));
    publishOutline();
}

void NotebookViewModel::publishOutline() {
    std::vector<OutlineItem> sections;
    sections.reserve(m_outline.sections().size());
    for (const core::SectionInfo& section : m_outline.sections()) {
        sections.push_back(OutlineItem{
            .title = QString::fromStdString(section.title),
            .thumbnail = {},
            .count = static_cast<int>(section.pages.size()),
        });
    }

    std::vector<OutlineItem> pages;
    if (const std::optional<std::size_t> section = currentSectionIndex()) {
        const std::vector<core::PageInfo>& infos = m_outline.sections()[*section].pages;
        pages.reserve(infos.size());
        for (std::size_t i = 0; i < infos.size(); ++i) {
            const auto revision = m_thumbnails.find(infos[i].id);
            const QString drawn = revision == m_thumbnails.end()
                                      ? QString{}
                                      : QStringLiteral("image://pages/%1-%2")
                                            .arg(QString::fromStdString(infos[i].id.toString()))
                                            .arg(revision->second);
            pages.push_back(OutlineItem{
                .title = infos[i].title.empty() ? tr("Page %1").arg(i + 1)
                                                : QString::fromStdString(infos[i].title),
                .thumbnail = drawn,
                .count = 0,
            });
        }
    }

    m_sectionsModel.setItems(std::move(sections));
    m_pagesModel.setItems(std::move(pages));
    emit outlineChanged();
}

void NotebookViewModel::refreshMedia() {
    if (m_canvas.isNull()) {
        return;
    }
    const core::PageInfo* const info = currentPageInfo();
    if (info == nullptr || !info->media) {
        m_openAsset = core::ContentId{};
        m_mediaScale = 0.0;
        m_shownMedia.erase(m_currentPage);
        if (m_shownMedia.empty()) {
            m_canvas->clearMedia();
        } else {
            publishMedia();
        }
        return;
    }
    if (info->media->asset == m_openAsset) {
        drawMedia();
        return;
    }
    m_shownMedia.erase(m_currentPage);
    const core::ContentId wanted = info->media->asset;
    publishMedia();
    if (!m_storage) {
        return;
    }
    const std::uint64_t opening = m_opening;
    m_storage->loadAsset(wanted, [this, opening](core::Result<core::Asset> asset) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, asset = std::move(asset)] mutable {
                showAsset(opening, std::move(asset));
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::showAsset(std::uint64_t opening, core::Result<core::Asset> asset) {
    if (opening != m_opening || m_canvas.isNull()) {
        return;
    }
    if (!asset) {
        reportError(QString::fromStdString(asset.error().message));
        return;
    }
    const core::PageInfo* const info = currentPageInfo();
    if (info == nullptr || !info->media || info->media->asset != asset->id) {
        return;
    }

    if (asset->kind == core::AssetKind::Image) {
        m_openAsset = asset->id;
        const auto bytes = std::make_shared<const QByteArray>(toByteArray(asset->data));
        const core::ContentId id = asset->id;
        if (m_pictures.joinable()) {
            m_pictures.join();
        }
        m_pictures = std::jthread{[this, opening, id, bytes] {
            try {
                QImage picture;
                if (!picture.loadFromData(*bytes)) {
                    picture = QImage{};
                }
                QMetaObject::invokeMethod(
                    this, [this, opening, id, picture] { showPicture(opening, id, picture); },
                    Qt::QueuedConnection);
            } catch (...) {
                qWarning("Reading a picture stopped unexpectedly");
            }
        }};
        return;
    }

    if (!m_pdf) {
        m_pdf.emplace();
    }
    m_openAsset = asset->id;
    const std::uint64_t generation = m_opening;
    const core::ContentId opened = m_openAsset;
    m_pdf->open(std::move(*asset), [this, generation, opened](
                                       core::Result<std::vector<platform::pdf::PageSize>> pages) {
        QMetaObject::invokeMethod(
            this,
            [this, generation, opened, pages = std::move(pages)] {
                if (generation != m_opening || !pages) {
                    return;
                }
                rememberDocument(opened, *pages);
                drawMedia();
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::showPicture(std::uint64_t opening, const core::ContentId& asset,
                                    const QImage& picture) {
    if (opening != m_opening || m_canvas.isNull() || asset != m_openAsset) {
        return;
    }
    if (picture.isNull()) {
        reportError(tr("The picture could not be read"));
        return;
    }
    const core::PageInfo* const info = currentPageInfo();
    const std::optional<core::PaperSize> paper =
        info == nullptr ? std::nullopt : core::paperSize(info->style);
    const QRectF area = paper ? QRectF{0.0, 0.0, static_cast<qreal>(paper->width),
                                       static_cast<qreal>(paper->height)}
                              : QRectF{0.0, 0.0, static_cast<qreal>(picture.width()),
                                       static_cast<qreal>(picture.height())};
    showPageMedia(m_currentPage, picture, area);
}

void NotebookViewModel::redrawMedia() {
    drawMedia();
    if (m_continuous) {
        wantNeighbours();
    }
}

void NotebookViewModel::drawMedia() {
    const core::PageInfo* const info = currentPageInfo();
    if (m_canvas.isNull() || info == nullptr || !info->media || !m_pdf) {
        return;
    }
    const std::optional<core::PaperSize> paper = sizeOfPage(*info);
    if (!paper) {
        return;
    }

    const qreal scale = drawScale(*paper, kSmallestMediaScale);
    const auto already = m_drawnAt.find(m_currentPage);
    const auto shown = m_shownMedia.find(m_currentPage);
    if (already != m_drawnAt.end() && already->second + kFineEnough >= scale
        && shown != m_shownMedia.end() && fitsSheet(shown->second.area, *paper)) {
        return;
    }

    m_mediaScale = scale;
    // The document page is drawn to fill the sheet it stands on, the same way the pages around
    // it are: a wider sheet shows a wider page, not a page with white space beside it.
    const core::ContentId asset = info->media->asset;
    drawColumnPage(m_currentPage, info->media->index, asset);
}

void NotebookViewModel::showPageMedia(const core::Uuid& page, const QImage& picture,
                                      const QRectF& area) {
    if (picture.isNull() || area.isEmpty()) {
        return;
    }
    const core::PageInfo* const info = m_outline.page(page);
    const std::optional<core::PaperSize> paper = info == nullptr ? std::nullopt : sizeOfPage(*info);
    // The page may have changed size while this was being drawn; then it is drawn again rather
    // than shown at a size it no longer has.
    if (paper && !fitsSheet(area, *paper)) {
        m_drawnAt.erase(page);
        if (page == m_currentPage) {
            drawMedia();
        } else if (info != nullptr) {
            wantMediaFor(*info);
        }
        return;
    }
    m_shownMedia.insert_or_assign(page, platform::ink::QtInkItem::MediaPiece{
                                            .page = page,
                                            .picture = picture,
                                            .area = area,
                                        });
    publishMedia();
}

void NotebookViewModel::publishMedia() {
    if (m_canvas.isNull()) {
        return;
    }
    std::vector<platform::ink::QtInkItem::MediaPiece> pieces;
    pieces.reserve(m_shownMedia.size());
    for (const auto& [page, piece] : m_shownMedia) {
        pieces.push_back(piece);
    }
    m_canvas->showMedia(pieces);
}

// While a page is being read only the part of its document that shows is drawn, in full detail.
// Once it is left behind, that part is all it has, so the column draws it whole again.
void NotebookViewModel::rememberDocument(const core::ContentId& asset,
                                         const std::vector<platform::pdf::PageSize>& sizes) {
    if (!sizes.empty()) {
        m_documentSizes.insert_or_assign(asset, sizes);
    }
}

std::optional<core::PaperSize> NotebookViewModel::sizeOfPage(const core::PageInfo& page) const {
    if (const std::optional<core::PaperSize> paper = core::paperSize(page.style)) {
        return paper;
    }
    if (!page.media) {
        return std::nullopt;
    }
    const auto document = m_documentSizes.find(page.media->asset);
    if (document == m_documentSizes.end()) {
        return std::nullopt;
    }
    const auto index = static_cast<std::size_t>(std::max(page.media->index, 0));
    if (index >= document->second.size()) {
        return std::nullopt;
    }
    const platform::pdf::PageSize& size = document->second[index];
    return core::PaperSize{.width = size.width, .height = size.height};
}

bool NotebookViewModel::hasWholeMedia(const core::PageInfo& page) const {
    const auto shown = m_shownMedia.find(page.id);
    if (shown == m_shownMedia.end()) {
        return false;
    }
    const std::optional<core::PaperSize> paper = sizeOfPage(page);
    if (!paper) {
        return true;
    }
    const auto drawn = m_drawnAt.find(page.id);
    if (drawn == m_drawnAt.end() || drawn->second * kMediaRedrawFactor < columnScale(*paper)) {
        return false;
    }
    return fitsSheet(shown->second.area, *paper);
}

// A page near the one being read shows its document as a whole, drawn once.
void NotebookViewModel::wantMediaFor(const core::PageInfo& page) {
    if (!page.media || !m_storage || page.id == m_currentPage || hasWholeMedia(page)) {
        return;
    }
    if (!m_wantedMedia.insert(page.id).second) {
        return;
    }
    const std::uint64_t opening = m_opening;
    const core::Uuid id = page.id;
    const core::PageStyle style = page.style;
    const int index = page.media->index;
    m_storage->loadAsset(
        page.media->asset, [this, opening, id, style, index](core::Result<core::Asset> asset) {
            QMetaObject::invokeMethod(
                this,
                [this, opening, id, style, index, asset = std::move(asset)] mutable {
                    m_wantedMedia.erase(id);
                    if (opening != m_opening || !asset) {
                        return;
                    }
                    drawColumnMedia(id, style, index, std::move(*asset));
                },
                Qt::QueuedConnection);
        });
}

// A sheet is drawn as finely as the reader is close to it, within what one picture may hold.
qreal NotebookViewModel::drawScale(const core::PaperSize& paper, qreal least) const {
    const qreal zoom = m_canvas.isNull() ? 1.0 : m_canvas->zoom();
    const qreal widest = double{kMaximumMediaPixels} / std::max(paper.width, paper.height);
    const qreal roomy =
        std::sqrt(double{kMostMediaPixels}
                  / (static_cast<double>(paper.width) * static_cast<double>(paper.height)));
    const qreal wanted = std::max({zoom, least, double{kSmallestMediaScale}});
    return std::min({wanted, widest, roomy});
}

qreal NotebookViewModel::columnScale(const core::PaperSize& paper) const {
    return drawScale(paper, kColumnMediaScale);
}

int NotebookViewModel::mediaPixelsOn(int index) const {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section) {
        return 0;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    const std::optional<std::size_t> at = checkedIndex(index, pages.size());
    if (!at) {
        return 0;
    }
    const auto shown = m_shownMedia.find(pages[*at].id);
    return shown == m_shownMedia.end() ? 0 : shown->second.picture.width();
}

void NotebookViewModel::drawColumnMedia(const core::Uuid& page, const core::PageStyle& style,
                                        int index, core::Asset asset) {
    if (asset.kind == core::AssetKind::Image) {
        QImage picture;
        if (!picture.loadFromData(toByteArray(asset.data))) {
            return;
        }
        const core::PageInfo* const info = m_outline.page(page);
        const std::optional<core::PaperSize> paper =
            info == nullptr ? core::paperSize(style) : sizeOfPage(*info);
        const QRectF area = paper ? QRectF{0.0, 0.0, static_cast<qreal>(paper->width),
                                           static_cast<qreal>(paper->height)}
                                  : QRectF{0.0, 0.0, static_cast<qreal>(picture.width()),
                                           static_cast<qreal>(picture.height())};
        showPageMedia(page, picture, area);
        return;
    }

    if (!m_pdf) {
        m_pdf.emplace();
    }
    // The document is opened first: a page with no paper of its own takes the size it was
    // written at, and that is only known once the document has been read.
    const core::ContentId id = asset.id;
    const std::uint64_t opening = m_opening;
    m_pdf->open(std::move(asset), [this, opening, id, page, index](
                                      core::Result<std::vector<platform::pdf::PageSize>> sizes) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, id, page, index, sizes = std::move(sizes)] {
                if (opening != m_opening || !sizes) {
                    return;
                }
                rememberDocument(id, *sizes);
                drawColumnPage(page, index, id);
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::drawColumnPage(const core::Uuid& page, int index,
                                       const core::ContentId& asset) {
    const core::PageInfo* const info = m_outline.page(page);
    if (info == nullptr || !m_pdf) {
        return;
    }
    const std::optional<core::PaperSize> paper = sizeOfPage(*info);
    if (!paper) {
        return;
    }
    const auto scale = static_cast<float>(columnScale(*paper));
    const auto width = static_cast<int>(
        std::clamp(paper->width * scale, 1.0F, static_cast<float>(kMaximumMediaPixels)));
    const auto height = static_cast<int>(
        std::clamp(paper->height * scale, 1.0F, static_cast<float>(kMaximumMediaPixels)));
    if (!m_drawing.insert(page).second) {
        return;
    }
    const QRectF area{0.0, 0.0, static_cast<qreal>(paper->width),
                      static_cast<qreal>(paper->height)};
    const std::uint64_t opening = m_opening;
    m_pdf->render(
        asset, index, width, height,
        [this, opening, page, area, scale](core::Result<platform::pdf::PageImage> image) mutable {
            QMetaObject::invokeMethod(
                this,
                [this, opening, page, area, scale, image = std::move(image)] {
                    if (opening != m_opening) {
                        return;
                    }
                    m_drawing.erase(page);
                    if (!image) {
                        return;
                    }
                    // Only a page that was really drawn counts as drawn.
                    m_drawnAt[page] = scale;
                    const QImage picture{image->pixels.data(), image->width, image->height,
                                         static_cast<qsizetype>(image->width) * 4,
                                         QImage::Format_RGBA8888};
                    showPageMedia(page, picture.copy(), area);
                },
                Qt::QueuedConnection);
        });
}

void NotebookViewModel::startFromDocument(const QUrl& fileUrl) {
    if (pageCount() == 1) {
        m_startingPage = m_currentPage;
    }
    importDocument(fileUrl);
}

// The empty page a new notebook opens with makes way for the pages that were read in.
void NotebookViewModel::dropStartingPage() {
    if (m_startingPage.isNil()) {
        return;
    }
    const core::Uuid going = std::exchange(m_startingPage, core::Uuid{});
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    if (pages.size() < 2) {
        return;
    }
    const auto found = std::ranges::find(pages, going, &core::PageInfo::id);
    if (found != pages.end()) {
        deletePage(static_cast<int>(std::distance(pages.begin(), found)));
    }
    emit documentStarted();
}

void NotebookViewModel::importDocument(const QUrl& fileUrl) {
    const std::optional<core::PagePlace> place = currentPlace();
    if (!place || !m_storage) {
        return;
    }
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) {
        reportError(tr("Could not open %1").arg(QFileInfo{path}.fileName()));
        return;
    }
    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        reportError(tr("%1 is empty").arg(QFileInfo{path}.fileName()));
        return;
    }

    const bool isPdf = data.startsWith("%PDF");
    core::Asset asset{
        .id = hashOf(data),
        .kind = isPdf ? core::AssetKind::Pdf : core::AssetKind::Image,
        .name = QFileInfo{path}.fileName().toStdString(),
        .data = toBytes(data),
    };

    if (!isPdf) {
        QImage image;
        if (!image.loadFromData(data)) {
            reportError(tr("%1 is neither a PDF nor a picture").arg(QFileInfo{path}.fileName()));
            return;
        }
        const core::PaperSize size{
            .width = static_cast<float>(image.width()),
            .height = static_cast<float>(image.height()),
        };
        const core::PageInfo page{
            .id = m_ids.next(),
            .title = asset.name,
            .style = core::styleForPaper(size),
            .media = core::PageMedia{.asset = asset.id, .index = 0},
        };
        std::vector<core::PageInfo> pages{page};
        runCommand(std::make_unique<core::ImportPagesCommand>(
            &m_outline, &*m_storage,
            core::PagePlace{.sectionId = place->sectionId, .index = place->index + 1},
            std::move(asset), std::move(pages)));
        dropStartingPage();
        return;
    }

    if (!m_pdf) {
        m_pdf.emplace();
    }
    const std::uint64_t opening = m_opening;
    m_pdf->open(asset, [this, opening,
                        asset](core::Result<std::vector<platform::pdf::PageSize>> sizes) mutable {
        QMetaObject::invokeMethod(
            this,
            [this, opening, asset = std::move(asset), sizes = std::move(sizes)] mutable {
                if (opening != m_opening) {
                    return;
                }
                if (!sizes) {
                    reportError(QString::fromStdString(sizes.error().message));
                    return;
                }
                const std::optional<core::PagePlace> place = currentPlace();
                if (!place || !m_storage || sizes->empty()) {
                    return;
                }
                std::vector<core::PageInfo> pages;
                pages.reserve(sizes->size());
                for (std::size_t index = 0; index < sizes->size(); ++index) {
                    const platform::pdf::PageSize& size = (*sizes)[index];
                    pages.push_back(core::PageInfo{
                        .id = m_ids.next(),
                        .title = {},
                        .style = core::styleForPaper(
                            core::PaperSize{.width = size.width, .height = size.height}),
                        .media =
                            core::PageMedia{.asset = asset.id, .index = static_cast<int>(index)},
                    });
                }
                m_openAsset = asset.id;
                runCommand(std::make_unique<core::ImportPagesCommand>(
                    &m_outline, &*m_storage,
                    core::PagePlace{.sectionId = place->sectionId, .index = place->index + 1},
                    std::move(asset), std::move(pages)));
                dropStartingPage();
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::refreshCanvas() {
    if (m_canvas.isNull()) {
        return;
    }
    if (m_continuous && currentSectionIndex()) {
        showColumn();
        return;
    }
    const core::PageInfo* const info = currentPageInfo();
    const core::PageStyle style = info == nullptr ? core::PageStyle{} : info->style;
    if (const core::Page* const page = currentPageData()) {
        std::vector<core::Stroke> pieces;
        for (const auto& [strokeId, left] : m_erasePieces) {
            pieces.insert(pieces.end(), left.begin(), left.end());
        }
        m_canvas->showPage(*page, style, m_erasing, pieces);
    } else {
        const core::Page placeholder{m_currentPage};
        m_canvas->showPage(placeholder, style);
    }
    if (const auto remembered = m_views.find(m_currentPage); remembered != m_views.end()) {
        m_canvas->showView(remembered->second);
    }
    publishTexts();
}

// The pages of the section stand in one column; the ones that are not read yet are empty sheets
// until their strokes arrive.
void NotebookViewModel::showColumn() {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section) {
        return;
    }
    const std::vector<core::PageInfo>& infos = m_outline.sections()[*section].pages;
    std::vector<core::Page> waiting;
    waiting.reserve(infos.size());
    std::vector<platform::ink::QtInkItem::PageView> views;
    views.reserve(infos.size());
    for (const core::PageInfo& info : infos) {
        const auto found = m_pages.find(info.id);
        const core::Page* page = found == m_pages.end() ? nullptr : found->second.get();
        if (page == nullptr) {
            waiting.emplace_back(info.id);
            page = &waiting.back();
        }
        views.push_back(platform::ink::QtInkItem::PageView{.page = page, .style = info.style});
    }

    std::vector<core::Stroke> pieces;
    for (const auto& [strokeId, left] : m_erasePieces) {
        pieces.insert(pieces.end(), left.begin(), left.end());
    }
    m_canvas->showColumn(views, currentPage(), m_erasing, pieces);
    publishTexts();
    wantNeighbours();
}

void NotebookViewModel::setContinuous(bool continuous) {
    if (continuous == m_continuous) {
        return;
    }
    m_continuous = continuous;
    m_shownMedia.clear();
    m_drawnAt.clear();
    m_wantedMedia.clear();
    m_drawing.clear();
    m_openAsset = core::ContentId{};
    m_mediaScale = 0.0;
    if (!m_canvas.isNull()) {
        m_canvas->clearMedia();
    }
    emit continuousChanged();
    refreshCanvas();
    refreshMedia();
}

void NotebookViewModel::goToShownPage(int index) {
    if (index != currentPage()) {
        setCurrentPage(index);
    }
}

// Only the pages around the one being read are held in full; the rest stay empty until they come
// near.
void NotebookViewModel::wantNeighbours() {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!section || !m_storage) {
        return;
    }
    const std::vector<core::PageInfo>& infos = m_outline.sections()[*section].pages;
    const int here = currentPage();
    const int first = std::max(0, here - kPagesAround);
    const int last = std::min(static_cast<int>(infos.size()) - 1, here + kPagesAround);
    for (int index = first; index <= last; ++index) {
        const core::PageInfo& info = infos[static_cast<std::size_t>(index)];
        const core::Uuid page = info.id;
        wantMediaFor(info);
        if (m_pages.contains(page) || !m_wantedPages.insert(page).second) {
            continue;
        }
        if (!m_storage) {
            return;
        }
        const std::uint64_t opening = m_opening;
        m_storage->loadPage(page, [this, opening, page](core::Result<core::LoadedPage> loaded) {
            QMetaObject::invokeMethod(
                this,
                [this, opening, page, loaded = std::move(loaded)] mutable {
                    m_wantedPages.erase(page);
                    if (opening != m_opening || !loaded || m_pages.contains(page)) {
                        return;
                    }
                    m_pages.emplace(page,
                                    std::make_unique<core::Page>(page, std::move(loaded->strokes),
                                                                 std::move(loaded->texts)));
                    refreshCanvas();
                },
                Qt::QueuedConnection);
        });
    }
    forgetFarMedia(infos, here);
}

// The documents of pages far from the one being read are let go of, and drawn again on the way
// back.
void NotebookViewModel::forgetFarMedia(std::span<const core::PageInfo> pages, int here) {
    std::set<core::Uuid> kept;
    const int first = std::max(0, here - kMediaAround);
    const int last = std::min(static_cast<int>(pages.size()) - 1, here + kMediaAround);
    for (int index = first; index <= last; ++index) {
        kept.insert(pages[static_cast<std::size_t>(index)].id);
    }
    std::map<core::Uuid, core::PaperSize> sheets;
    for (const core::PageInfo& page : pages) {
        if (const std::optional<core::PaperSize> paper = core::paperSize(page.style)) {
            sheets.emplace(page.id, *paper);
        }
    }

    bool dropped = false;
    for (auto piece = m_shownMedia.begin(); piece != m_shownMedia.end();) {
        const auto sheet = sheets.find(piece->first);
        // A page keeps its picture while it is near and while the picture still fits its sheet.
        const bool stale = sheet != sheets.end() && !fitsSheet(piece->second.area, sheet->second);
        if (kept.contains(piece->first) && !stale) {
            ++piece;
            continue;
        }
        m_drawnAt.erase(piece->first);
        piece = m_shownMedia.erase(piece);
        dropped = true;
    }
    if (dropped) {
        publishMedia();
    }
}

void NotebookViewModel::exportToPdf(const QUrl& fileUrl, int scope) {
    if (m_exporting || !m_storage || m_notebookPath.isEmpty()) {
        return;
    }
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (path.isEmpty()) {
        return;
    }

    m_storage->waitUntilIdle();
    m_exporting = true;
    emit exportingChanged();

    const auto job = std::make_shared<ExportJob>(ExportJob{
        .notebook = std::filesystem::path{m_notebookPath.toStdU16String()},
        .target = std::filesystem::path{path.toStdU16String()},
        .path = path,
        .scope = static_cast<platform::render::ExportScope>(
            std::clamp(scope, 0, static_cast<int>(platform::render::ExportScope::Document))),
    });
    m_export = std::jthread{[this, job] {
        try {
            core::Result<int> written = platform::render::exportNotebookToPdf(
                job->notebook, job->target, platform::render::ExportOptions{.scope = job->scope});
            QMetaObject::invokeMethod(
                this,
                [this, job, written = std::move(written)] { finishExport(job->path, written); },
                Qt::QueuedConnection);
        } catch (...) {
            qWarning("Writing %s stopped unexpectedly", qUtf8Printable(job->path));
        }
    }};
}

bool NotebookViewModel::save() {
    if (m_keptAt.isEmpty()) {
        return false;
    }
    return writeTo(m_keptAt);
}

void NotebookViewModel::saveAs(const QUrl& fileUrl) {
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (path.isEmpty() || !writeTo(path)) {
        return;
    }
    if (path != m_keptAt) {
        m_keptAt = path;
        emit keptAtChanged();
        QSettings settings;
        settings.setValue(QString{kKeptPrefix} + name(), path);
    }
    emit nameWanted(QFileInfo{path}.completeBaseName());
}

bool NotebookViewModel::writeTo(const QString& path) {
    if (!m_storage || m_notebookPath.isEmpty()) {
        return false;
    }
    m_storage->submit([](core::NotebookStore& store) { return store.checkpoint(); });
    m_storage->waitUntilIdle();

    if (path != m_notebookPath) {
        QFile source{m_notebookPath};
        if (QFile::exists(path) && !QFile::remove(path)) {
            reportError(tr("Could not write %1").arg(QFileInfo{path}.fileName()));
            return false;
        }
        if (!source.copy(path)) {
            reportError(
                tr("Could not write %1: %2").arg(QFileInfo{path}.fileName(), source.errorString()));
            return false;
        }
    }
    if (m_edited) {
        m_edited = false;
        emit editedChanged();
    }
    emit saved(path);
    return true;
}

bool NotebookViewModel::readsHandwriting() {
    return HandwritingReader::availableHere();
}

void NotebookViewModel::find(const QString& text) {
    if (!m_storage || text.trimmed().isEmpty()) {
        emit found(QVariantList{});
        return;
    }
    m_storage->submit(
        [this, wanted = text.toStdString()](core::NotebookStore& store) -> core::Result<void> {
            core::Result<std::vector<core::FoundWord>> hits = store.findWords(wanted);
            if (!hits) {
                return std::unexpected{hits.error()};
            }
            QMetaObject::invokeMethod(
                this, [this, hits = std::move(*hits)] mutable { publishFound(std::move(hits)); },
                Qt::QueuedConnection);
            return {};
        });
}

void NotebookViewModel::goToFound(int index) {
    const std::optional<std::size_t> at = checkedIndex(index, m_found.size());
    if (!at) {
        return;
    }
    goToPage(m_found[*at].pageId);
}

void NotebookViewModel::publishFound(std::vector<core::FoundWord> hits) {
    m_found = std::move(hits);
    QVariantList results;
    results.reserve(static_cast<qsizetype>(std::min(m_found.size(), kMostFound)));
    for (const core::FoundWord& hit : m_found) {
        if (std::cmp_greater_equal(results.size(), kMostFound)) {
            break;
        }
        const core::PageInfo* const page = m_outline.page(hit.pageId);
        if (page == nullptr) {
            continue;
        }
        QString section;
        if (const std::optional<core::PagePlace> place = m_outline.placeOf(hit.pageId)) {
            const std::optional<std::size_t> index = m_outline.sectionIndex(place->sectionId);
            if (index) {
                section = QString::fromStdString(m_outline.sections()[*index].title);
            }
        }
        results.append(QVariantMap{
            {QStringLiteral("pageId"), QString::fromStdString(hit.pageId.toString())},
            {QStringLiteral("pageTitle"), QString::fromStdString(page->title)},
            {QStringLiteral("sectionTitle"), section},
            {QStringLiteral("text"), QString::fromStdString(hit.word.text)},
            {QStringLiteral("left"), hit.word.box.left},
            {QStringLiteral("top"), hit.word.box.top},
            {QStringLiteral("right"), hit.word.box.right},
            {QStringLiteral("bottom"), hit.word.box.bottom},
        });
    }
    emit found(results);
}

void NotebookViewModel::markEdited() {
    if (m_edited || !m_loaded) {
        return;
    }
    m_edited = true;
    emit editedChanged();
}

void NotebookViewModel::finishExport(const QString& path, const core::Result<int>& written) {
    m_exporting = false;
    emit exportingChanged();
    if (!written) {
        reportError(tr("The notebook could not be written: %1")
                        .arg(QString::fromStdString(written.error().message)));
        return;
    }
    emit exported(path);
}

void NotebookViewModel::selectInside(std::span<const core::Point> polygon) {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull()) {
        return;
    }
    m_canvas->showSelection(core::strokesInside(*page, polygon));
}

void NotebookViewModel::moveSelection(float dx, float dy) {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !m_storage) {
        return;
    }
    std::vector<core::Uuid> picked = m_canvas->selection();
    if (picked.empty()) {
        return;
    }
    m_canvas->forgetStrokes(picked);
    runCommand(
        std::make_unique<core::MoveStrokesCommand>(page, &*m_storage, std::move(picked), dx, dy));
}

void NotebookViewModel::deleteSelection() {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !m_storage) {
        return;
    }
    std::vector<core::Uuid> picked = m_canvas->selection();
    if (picked.empty()) {
        return;
    }
    m_canvas->showSelection({});
    m_canvas->forgetStrokes(picked);
    runCommand(std::make_unique<core::EraseStrokesCommand>(page, &*m_storage, std::move(picked)));
}

void NotebookViewModel::copySelection() {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull()) {
        return;
    }
    const std::vector<core::Uuid>& picked = m_canvas->selection();
    std::vector<core::Stroke> copies;
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) != picked.end()) {
            copies.push_back(placed.stroke);
        }
    }
    if (copies.empty()) {
        return;
    }
    m_clipboard = std::move(copies);
    emit clipboardChanged();
}

void NotebookViewModel::copySelectionAsText() {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull()) {
        return;
    }
    const std::vector<core::Uuid>& picked = m_canvas->selection();
    std::vector<core::Stroke> written;
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) != picked.end()) {
            written.push_back(placed.stroke);
        }
    }
    if (written.empty()) {
        return;
    }
    m_reader.readSoon(std::move(written), [this](core::Result<std::vector<core::InkWord>> words) {
        if (!words) {
            reportError(QString::fromStdString(words.error().message));
            return;
        }
        QString text;
        for (const core::InkWord& word : *words) {
            if (!text.isEmpty()) {
                text.append(QLatin1Char{' '});
            }
            text.append(QString::fromStdString(word.text));
        }
        if (text.isEmpty()) {
            reportError(tr("Nothing there could be read as words"));
            return;
        }
        QGuiApplication::clipboard()->setText(text);
        emit copiedAsText(text);
    });
}

void NotebookViewModel::pasteStrokes() {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !m_storage || m_clipboard.empty()) {
        return;
    }

    std::int64_t ordinal = page->nextOrdinal();
    std::vector<core::PlacedStroke> pasted;
    std::vector<core::Uuid> ids;
    pasted.reserve(m_clipboard.size());
    ids.reserve(m_clipboard.size());
    for (const core::Stroke& stroke : m_clipboard) {
        const core::Stroke shifted = core::moved(stroke, kPasteOffset, kPasteOffset);
        core::Stroke fresh{m_ids.next(), shifted.style()};
        for (const core::InkSample& sample : shifted.samples()) {
            fresh.append(sample);
        }
        ids.push_back(fresh.id());
        pasted.push_back(core::PlacedStroke{.ordinal = ordinal, .stroke = std::move(fresh)});
        ++ordinal;
    }

    runCommand(std::make_unique<core::AddStrokesCommand>(page, &*m_storage, std::move(pasted)));
    m_clipboard.clear();
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(ids, placed.stroke.id()) != ids.end()) {
            m_clipboard.push_back(core::moved(placed.stroke, -kPasteOffset, -kPasteOffset));
        }
    }
    m_canvas->showSelection(std::move(ids));
}

void NotebookViewModel::recolourSelection(const QColor& color) {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !m_storage) {
        return;
    }
    std::vector<core::Uuid> picked = m_canvas->selection();
    if (picked.empty()) {
        return;
    }

    const core::PlacedStroke* first = nullptr;
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (placed.stroke.id() == picked.front()) {
            first = &placed;
            break;
        }
    }
    if (first == nullptr) {
        return;
    }
    core::StrokeStyle style = first->stroke.style();
    style.color = core::Color{
        .red = static_cast<std::uint8_t>(color.red()),
        .green = static_cast<std::uint8_t>(color.green()),
        .blue = static_cast<std::uint8_t>(color.blue()),
        .alpha = static_cast<std::uint8_t>(color.alpha()),
    };

    m_canvas->forgetStrokes(picked);
    runCommand(
        std::make_unique<core::RestyleStrokesCommand>(page, &*m_storage, std::move(picked), style));
}

void NotebookViewModel::refreshTrash() {
    if (!m_storage) {
        return;
    }
    const std::uint64_t opening = m_opening;
    m_storage->loadTrash([this, opening](core::Result<std::vector<core::TrashedItem>> items) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, items = std::move(items)] mutable {
                showTrash(opening, std::move(items));
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::showTrash(std::uint64_t opening,
                                  core::Result<std::vector<core::TrashedItem>> items) {
    if (opening != m_opening) {
        return;
    }
    if (!items) {
        reportError(QString::fromStdString(items.error().message));
        return;
    }
    m_trashed = std::move(*items);

    std::vector<TrashItem> shown;
    shown.reserve(m_trashed.size());
    for (const core::TrashedItem& item : m_trashed) {
        QString title = QString::fromStdString(item.title);
        if (title.isEmpty()) {
            title = item.wholeSection ? tr("Section without a name") : tr("Page without a name");
        }
        shown.push_back(TrashItem{
            .title = title,
            .wholeSection = item.wholeSection,
            .restorable = item.wholeSection || !item.sectionTrashed,
        });
    }
    m_trashModel.setItems(std::move(shown));
}

void NotebookViewModel::restoreTrashed(int index) {
    const std::optional<std::size_t> at = checkedIndex(index, m_trashed.size());
    if (!at || !m_storage) {
        return;
    }
    const core::TrashedItem item = m_trashed[*at];
    if (!item.wholeSection && item.sectionTrashed) {
        reportError(tr("Put the section back first"));
        return;
    }

    if (item.wholeSection) {
        std::vector<core::Uuid> order;
        order.reserve(m_outline.sections().size() + 1);
        for (const core::SectionInfo& section : m_outline.sections()) {
            order.push_back(section.id);
        }
        order.push_back(item.id);
        m_storage->submit([id = item.id, order](core::NotebookStore& store) {
            return store.restoreSection(id, order);
        });
    } else {
        const std::optional<std::size_t> section = m_outline.sectionIndex(item.sectionId);
        if (!section) {
            reportError(tr("The section this page was in is gone"));
            return;
        }
        std::vector<core::Uuid> order;
        for (const core::PageInfo& page : m_outline.sections()[*section].pages) {
            order.push_back(page.id);
        }
        order.push_back(item.id);
        m_storage->submit(
            [sectionId = item.sectionId, id = item.id, order](core::NotebookStore& store) {
                return store.restorePage(sectionId, id, order);
            });
    }

    m_history.clear();
    markEdited();
    emit historyChanged();
    reloadOutline();
    refreshTrash();
}

void NotebookViewModel::emptyTrash() {
    if (!m_storage) {
        return;
    }
    m_storage->submit([](core::NotebookStore& store) { return store.emptyTrash(); });
    refreshTrash();
}

void NotebookViewModel::reloadOutline() {
    if (!m_storage) {
        return;
    }
    const std::uint64_t opening = m_opening;
    m_storage->loadOutline([this, opening](core::Result<core::NotebookOutline> outline) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, outline = std::move(outline)] mutable {
                applyOutline(opening, std::move(outline));
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::applyOutline(std::uint64_t opening,
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
    if (m_outline.placeOf(m_currentPage)) {
        emit currentPageChanged();
        return;
    }
    if (!m_outline.sections().empty() && !m_outline.sections().front().pages.empty()) {
        goToPage(m_outline.sections().front().pages.front().id);
    }
}

void NotebookViewModel::reportError(const QString& message) {
    qWarning("%s", qUtf8Printable(message));
    m_errorMessage = message;
    emit errorMessageChanged();
}

int NotebookViewModel::sheetCount() const {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!m_continuous || !section) {
        return 1;
    }
    return static_cast<int>(m_outline.sections()[*section].pages.size());
}

int NotebookViewModel::sheetOfPage(const core::Uuid& pageId) const {
    const std::optional<std::size_t> section = currentSectionIndex();
    if (!m_continuous || !section) {
        return 0;
    }
    const std::vector<core::PageInfo>& pages = m_outline.sections()[*section].pages;
    for (std::size_t at = 0; at < pages.size(); ++at) {
        if (pages[at].id == pageId) {
            return static_cast<int>(at);
        }
    }
    return 0;
}

std::optional<NotebookViewModel::TextPlace> NotebookViewModel::placeInColumn(QPointF column) const {
    if (m_canvas.isNull()) {
        return std::nullopt;
    }
    const int sheets = sheetCount();
    for (int sheet = 0; sheet < sheets; ++sheet) {
        const QRectF where = m_canvas->sheetRect(sheet);
        if (where.isEmpty() || !where.contains(column)) {
            continue;
        }
        return TextPlace{
            .page = pageOfSheet(sheet),
            .at =
                core::Point{
                    .x = static_cast<float>(column.x() - where.x()),
                    .y = static_cast<float>(column.y() - where.y()),
                },
            .sheet = sheet,
        };
    }
    return std::nullopt;
}

std::optional<std::pair<core::Uuid, core::TextBox>>
NotebookViewModel::textById(const QString& textId) const {
    if (m_draft && QString::fromStdString(m_draft->box.id.toString()) == textId) {
        return std::pair{m_draft->page, m_draft->box};
    }
    for (const auto& [pageId, page] : m_pages) {
        for (const core::PlacedText& placed : page->texts()) {
            if (QString::fromStdString(placed.box.id.toString()) == textId) {
                return std::pair{pageId, placed.box};
            }
        }
    }
    return std::nullopt;
}

void NotebookViewModel::setPickedText(const QString& textId) {
    if (textId == m_pickedText) {
        return;
    }
    m_pickedText = textId;
    emit pickedTextChanged();
    emit pickedBoxChanged();
}

QVariantMap NotebookViewModel::pickedBox() const {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(m_pickedText);
    if (!found) {
        return {};
    }
    const core::TextBox& box = found->second;
    QVariantMap map = mapOfStyle(box.style);
    const QRectF where =
        m_canvas.isNull() ? QRectF{} : m_canvas->sheetRect(sheetOfPage(found->first));
    map.insert("textId", m_pickedText);
    map.insert("text", QString::fromStdString(box.text));
    map.insert("columnX", where.x() + static_cast<qreal>(box.at.x));
    map.insert("columnY", where.y() + static_cast<qreal>(box.at.y));
    map.insert("boxWidth", static_cast<qreal>(box.width));
    map.insert("boxHeight", static_cast<qreal>(box.height));
    return map;
}

void NotebookViewModel::publishTexts() {
    std::vector<TextItem> items;
    if (!m_canvas.isNull()) {
        const int sheets = sheetCount();
        for (int sheet = 0; sheet < sheets; ++sheet) {
            const core::Uuid pageId = pageOfSheet(sheet);
            const auto found = m_pages.find(pageId);
            if (found == m_pages.end()) {
                continue;
            }
            const QRectF where = m_canvas->sheetRect(sheet);
            const QString page = QString::fromStdString(pageId.toString());
            std::vector<core::TextBox> boxes;
            for (const core::PlacedText& placed : found->second->texts()) {
                boxes.push_back(placed.box);
            }
            if (m_draft && m_draft->page == pageId) {
                boxes.push_back(m_draft->box);
            }
            for (const core::TextBox& box : boxes) {
                const core::Color colour = box.style.color;
                items.push_back(TextItem{
                    .textId = QString::fromStdString(box.id.toString()),
                    .pageId = page,
                    .text = QString::fromStdString(box.text),
                    .font = QString::fromStdString(box.style.font),
                    .color = QColor::fromRgb(colour.red, colour.green, colour.blue, colour.alpha),
                    .columnX = where.x() + static_cast<qreal>(box.at.x),
                    .columnY = where.y() + static_cast<qreal>(box.at.y),
                    .width = static_cast<qreal>(box.width),
                    .height = static_cast<qreal>(box.height),
                    .size = static_cast<qreal>(box.style.size),
                    .lineHeight = static_cast<qreal>(box.style.lineHeight),
                    .align = static_cast<int>(box.style.align),
                    .sheet = sheet,
                    .bold = box.style.bold,
                    .italic = box.style.italic,
                    .underline = box.style.underline,
                    .struckOut = box.style.struckOut,
                });
            }
        }
    }
    m_textsModel.setItems(std::move(items));
    emit pickedBoxChanged();
}

void NotebookViewModel::changeText(const core::Uuid& pageId, core::TextBox box) {
    if (m_draft && m_draft->box.id == box.id) {
        m_draft->box = core::normalized(std::move(box));
        publishTexts();
        return;
    }
    const auto found = m_pages.find(pageId);
    if (found == m_pages.end() || !m_storage) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(pageId);
    runCommand(std::make_unique<core::ChangeTextCommand>(found->second.get(), storage,
                                                         core::normalized(std::move(box))));
    publishTexts();
}

void NotebookViewModel::settleDraft() {
    if (!m_draft) {
        return;
    }
    const Draft draft = *m_draft;
    m_draft.reset();
    const auto found = m_pages.find(draft.page);
    if (draft.box.text.empty() || found == m_pages.end() || !m_storage) {
        return;
    }
    core::Page* const page = found->second.get();
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(draft.page);
    runCommand(std::make_unique<core::AddTextCommand>(
        page, storage, core::PlacedText{.ordinal = page->nextTextOrdinal(), .box = draft.box}));
}

void NotebookViewModel::addTextAt(qreal columnX, qreal columnY, const QVariantMap& style) {
    settleDraft();
    const std::optional<TextPlace> place = placeInColumn(QPointF{columnX, columnY});
    if (!place || !m_storage || m_canvas.isNull()) {
        publishTexts();
        return;
    }
    const auto found = m_pages.find(place->page);
    if (found == m_pages.end()) {
        publishTexts();
        return;
    }

    const core::TextStyle face = styleOfMap(style);
    const qreal room = m_canvas->sheetRect(place->sheet).width() - columnX
                       + m_canvas->sheetRect(place->sheet).x() - kTextMargin;
    core::TextBox box{
        .id = m_ids.next(),
        .at = place->at,
        .width = static_cast<float>(std::max(std::min(room, kNewTextWidth), 0.0)),
        .height = core::pageUnitsOfPoints(face.size) * face.lineHeight,
        .text = {},
        .style = face,
    };
    box = core::normalized(std::move(box));

    m_draft = Draft{.page = place->page, .box = box};
    publishTexts();
    const QString textId = QString::fromStdString(box.id.toString());
    setPickedText(textId);
    emit textAdded(textId);
}

void NotebookViewModel::finishText(const QString& textId, const QString& text, qreal height) {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    if (!found) {
        return;
    }
    if (text.isEmpty()) {
        removeText(textId);
        return;
    }
    const bool draft = m_draft && QString::fromStdString(m_draft->box.id.toString()) == textId;
    core::TextBox box = found->second;
    const std::string wanted = text.toStdString();
    const auto tall = static_cast<float>(height);
    if (wanted == box.text && qFuzzyCompare(tall + 1.0F, box.height + 1.0F)) {
        return;
    }
    box.text = wanted;
    box.height = tall;
    changeText(found->first, std::move(box));
    if (draft) {
        settleDraft();
        publishTexts();
    }
}

void NotebookViewModel::placeText(const QString& textId, qreal columnX, qreal columnY, qreal width,
                                  qreal height) {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    if (!found || m_canvas.isNull()) {
        return;
    }
    const QRectF where = m_canvas->sheetRect(sheetOfPage(found->first));
    core::TextBox box = found->second;
    const core::Point at{
        .x = static_cast<float>(columnX - where.x()),
        .y = static_cast<float>(columnY - where.y()),
    };
    const auto wide = static_cast<float>(width);
    const auto tall = static_cast<float>(height);
    if (at == box.at && qFuzzyCompare(wide + 1.0F, box.width + 1.0F)
        && qFuzzyCompare(tall + 1.0F, box.height + 1.0F)) {
        return;
    }
    box.at = at;
    box.width = wide;
    box.height = tall;
    changeText(found->first, std::move(box));
}

void NotebookViewModel::styleText(const QString& textId, const QVariantMap& style) {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    if (!found) {
        return;
    }
    core::TextBox box = found->second;
    const core::TextStyle face = styleOfMap(style);
    if (face == box.style) {
        return;
    }
    box.style = face;
    changeText(found->first, std::move(box));
}

void NotebookViewModel::removeText(const QString& textId) {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    if (!found || !m_storage) {
        return;
    }
    if (m_draft && QString::fromStdString(m_draft->box.id.toString()) == textId) {
        m_draft.reset();
        if (textId == m_pickedText) {
            setPickedText({});
        }
        publishTexts();
        return;
    }
    const auto page = m_pages.find(found->first);
    if (page == m_pages.end()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    if (textId == m_pickedText) {
        setPickedText({});
    }
    forgetThumbnail(found->first);
    runCommand(
        std::make_unique<core::RemoveTextCommand>(page->second.get(), storage, found->second.id));
    publishTexts();
}

QVariantMap NotebookViewModel::styleOfText(const QString& textId) const {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    return found ? mapOfStyle(found->second.style) : QVariantMap{};
}

void NotebookViewModel::convertSelectionToText(QVariantMap style) {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !m_storage) {
        return;
    }
    std::vector<core::Uuid> picked = m_canvas->selection();
    std::vector<core::Stroke> written;
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) != picked.end()) {
            written.push_back(placed.stroke);
        }
    }
    if (written.empty()) {
        return;
    }

    const core::Uuid pageId = m_currentPage;
    m_reader.readSoon(std::move(written), [this, style = std::move(style),
                                           picked = std::move(picked),
                                           pageId](core::Result<std::vector<core::InkWord>> words) {
        if (!words) {
            reportError(QString::fromStdString(words.error().message));
            return;
        }
        const std::optional<core::TextBlock> block = core::textOf(*words);
        if (!block) {
            reportError(tr("Nothing there could be read as words"));
            return;
        }
        const auto found = m_pages.find(pageId);
        if (found == m_pages.end() || !m_storage) {
            return;
        }

        core::TextStyle face = styleOfMap(style);
        // Type of about the size of the hand that wrote it, so the page reads as it did.
        face.size = block->size;
        core::Page* const on = found->second.get();
        core::TextBox box = core::normalized(core::TextBox{
            .id = m_ids.next(),
            .at = core::Point{.x = block->area.left, .y = block->area.top},
            .width = block->width,
            .height = block->area.height(),
            .text = block->text,
            .style = face,
        });

        std::vector<std::unique_ptr<core::ICommand>> steps;
        steps.push_back(std::make_unique<core::EraseStrokesCommand>(on, &*m_storage, picked));
        steps.push_back(std::make_unique<core::AddTextCommand>(
            on, &*m_storage, core::PlacedText{.ordinal = on->nextTextOrdinal(), .box = box}));
        forgetThumbnail(pageId);
        m_canvas->clearSelection();
        runCommand(std::make_unique<core::BundleCommand>(std::move(steps)));
        publishTexts();
        setPickedText(QString::fromStdString(box.id.toString()));
    });
}

}
