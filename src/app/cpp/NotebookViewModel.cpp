#include "app/cpp/NotebookViewModel.hpp"

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
#include "core/undo/OutlineCommands.hpp"
#include "core/undo/StrokeCommands.hpp"
#include "platform/pdf/PdfRenderer.hpp"
#include "platform/render/PagePainter.hpp"
#include "platform/render/PdfExporter.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
constexpr auto kKeptPrefix = "kept/";
constexpr int kMaximumMediaPixels = 4096;
constexpr qreal kMediaRedrawFactor = 1.4;
constexpr int kMediaRedrawDelay = 200;
constexpr float kPasteOffset = 24.0F;
constexpr float kPickRadius = 6.0F;
constexpr float kOwnPaperWidth = core::millimeters(210.0F);
constexpr float kOwnPaperHeight = core::millimeters(297.0F);
constexpr int kPagesAround = 2;
constexpr int kMediaAround = 4;

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
}

NotebookViewModel::NotebookViewModel(QString path, QString startPage, QObject* parent)
    : QObject(parent), m_notebookPath{std::move(path)}, m_startPage{std::move(startPage)},
      m_completed{true} {
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
        std::error_code ignored;
        std::filesystem::rename(from.native() + suffix, to.native() + suffix, ignored);
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
    // A page that carries a document keeps the size that document was written at: endless paper
    // would leave it nothing to stand on in the column.
    const bool endless = !core::paperSize(style).has_value();
    std::vector<core::Uuid> wanted;
    for (const core::PageInfo& page : m_outline.sections()[*section].pages) {
        if (endless && page.media) {
            continue;
        }
        if (page.style != style) {
            wanted.push_back(page.id);
        }
    }
    if (wanted.empty()) {
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

void NotebookViewModel::Sink::colourWanted(const core::InkSample& at) {
    m_owner->pickColour(at);
}

void NotebookViewModel::Sink::strokeCompleted(const core::Stroke& stroke) {
    m_owner->storeStroke(stroke);
}

void NotebookViewModel::Sink::strokeCancelled() {}

void NotebookViewModel::Sink::eraserMoved(const core::InkSample& from, const core::InkSample& to,
                                          float radius) {
    m_owner->erase(from, to, radius);
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

void NotebookViewModel::storeStroke(const core::Stroke& stroke) {
    forgetThumbnail(m_currentPage);
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
    markEdited();
    emit historyChanged();
}

void NotebookViewModel::pickColour(const core::InkSample& at) {
    const core::Page* const page = currentPageData();
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
    core::Page* const page = currentPageData();
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

void NotebookViewModel::addPage() {
    const std::optional<core::PagePlace> place = currentPlace();
    const core::PageInfo* const info = currentPageInfo();
    if (!place || info == nullptr || !m_storage) {
        return;
    }
    const core::PageInfo page{
        .id = m_ids.next(),
        .title = {},
        .style = info->style,
        .media = std::nullopt,
    };
    m_pages.emplace(page.id, std::make_unique<core::Page>(page.id));
    runCommand(std::make_unique<core::AddPageCommand>(
        &m_outline, &*m_storage,
        core::PagePlace{.sectionId = place->sectionId, .index = place->index + 1}, page));
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
        copyPage(original, cached->second->strokes());
        return;
    }

    const std::uint64_t opening = m_opening;
    const auto wanted = std::make_shared<const core::PageInfo>(original);
    m_storage->loadPage(
        wanted->id, [this, opening, wanted](core::Result<std::vector<core::PlacedStroke>> strokes) {
            QMetaObject::invokeMethod(
                this,
                [this, opening, wanted, strokes = std::move(strokes)] {
                    if (opening != m_opening || !strokes) {
                        return;
                    }
                    copyPage(*wanted, *strokes);
                },
                Qt::QueuedConnection);
        });
}

void NotebookViewModel::copyPage(const core::PageInfo& original,
                                 std::span<const core::PlacedStroke> strokes) {
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
    m_pages.insert_or_assign(copy.id, std::move(page));

    runCommand(std::make_unique<core::DuplicatePageCommand>(
        &m_outline, &*m_storage,
        core::PagePlace{.sectionId = place->sectionId, .index = place->index + 1}, std::move(copy),
        std::move(copies)));
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
        .media = {},
    });
    if (const auto cached = m_pages.find(work->page.id); cached != m_pages.end()) {
        const std::span<const core::PlacedStroke> strokes = cached->second->strokes();
        work->strokes.assign(strokes.begin(), strokes.end());
        gatherThumbnail(work);
        return;
    }

    const std::uint64_t opening = m_opening;
    m_storage->loadPage(work->page.id, [this, opening, work](
                                           core::Result<std::vector<core::PlacedStroke>> strokes) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, work, strokes = std::move(strokes)] mutable {
                if (opening != m_opening || !strokes) {
                    return;
                }
                work->strokes = std::move(*strokes);
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
        .media = work.media.isNull() ? nullptr : &work.media,
    };
    const core::Rect area = platform::render::pageArea(contents);
    const bool anything = area.width() > 0.0F && area.height() > 0.0F;
    // A page with nothing on it is still a page: it shows as the empty sheet it is.
    const float ratio = anything ? area.height() / area.width() : kEmptyPageRatio;
    const int height =
        std::max(1, static_cast<int>(static_cast<float>(thumbnails::kWidth) * ratio));
    QImage picture{thumbnails::kWidth, height, QImage::Format_ARGB32_Premultiplied};
    picture.fill(Qt::white);
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
    m_pdf->open(std::move(*asset),
                [this, generation](core::Result<std::vector<platform::pdf::PageSize>> pages) {
                    QMetaObject::invokeMethod(
                        this,
                        [this, generation, ok = pages.has_value()] {
                            if (generation == m_opening && ok) {
                                drawMedia();
                            }
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
    const std::optional<core::PaperSize> paper = core::paperSize(info->style);
    if (!paper) {
        return;
    }

    const core::Rect wanted = wantedRegion(*paper);
    const qreal scale = drawScale(*paper, kSmallestMediaScale);
    const auto already = m_drawnAt.find(m_currentPage);
    const auto shown = m_shownMedia.find(m_currentPage);
    if (already != m_drawnAt.end() && already->second + kFineEnough >= scale
        && shown != m_shownMedia.end() && fitsSheet(shown->second.area, *paper)) {
        return;
    }
    const auto width = static_cast<int>(std::max(1.0, static_cast<double>(wanted.width()) * scale));
    const auto height =
        static_cast<int>(std::max(1.0, static_cast<double>(wanted.height()) * scale));

    m_mediaScale = scale;
    m_drawnAt[m_currentPage] = scale;
    const std::uint64_t opening = m_opening;
    const core::ContentId asset = info->media->asset;
    const platform::pdf::PageRegion region{
        .left = wanted.left,
        .top = wanted.top,
        .width = wanted.width(),
        .height = wanted.height(),
    };
    m_pdf->renderRegion(
        asset, info->media->index, width, height, region,
        [this, opening, asset, wanted](core::Result<platform::pdf::PageImage> image) {
            if (!image) {
                return;
            }
            QMetaObject::invokeMethod(
                this,
                [this, opening, asset, wanted, drawn = std::move(*image)] {
                    showRenderedPage(opening, asset, wanted, drawn);
                },
                Qt::QueuedConnection);
        });
}

// The sheet is drawn whole, as finely as the reader is close, and never in pieces: half a page
// of a document is worse than a slightly coarse one.
core::Rect NotebookViewModel::wantedRegion(const core::PaperSize& paper) {
    return core::Rect{
        .left = 0.0F,
        .top = 0.0F,
        .right = paper.width,
        .bottom = paper.height,
    };
}

void NotebookViewModel::showRenderedPage(std::uint64_t opening, const core::ContentId& asset,
                                         const core::Rect& area,
                                         const platform::pdf::PageImage& image) {
    const core::PageInfo* const info = currentPageInfo();
    if (opening != m_opening || m_canvas.isNull() || info == nullptr || !info->media
        || info->media->asset != asset) {
        return;
    }
    const QImage drawn{image.pixels.data(), image.width, image.height,
                       static_cast<qsizetype>(image.width) * 4, QImage::Format_RGBA8888};
    showPageMedia(m_currentPage, drawn.copy(),
                  QRectF{area.left, area.top, area.width(), area.height()});
}

void NotebookViewModel::showPageMedia(const core::Uuid& page, const QImage& picture,
                                      const QRectF& area) {
    if (picture.isNull() || area.isEmpty()) {
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
bool NotebookViewModel::hasWholeMedia(const core::PageInfo& page) const {
    const auto shown = m_shownMedia.find(page.id);
    if (shown == m_shownMedia.end()) {
        return false;
    }
    const std::optional<core::PaperSize> paper = core::paperSize(page.style);
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
    const std::optional<core::PaperSize> paper = core::paperSize(style);
    if (asset.kind == core::AssetKind::Image) {
        QImage picture;
        if (!picture.loadFromData(toByteArray(asset.data))) {
            return;
        }
        const QRectF area = paper ? QRectF{0.0, 0.0, static_cast<qreal>(paper->width),
                                           static_cast<qreal>(paper->height)}
                                  : QRectF{0.0, 0.0, static_cast<qreal>(picture.width()),
                                           static_cast<qreal>(picture.height())};
        showPageMedia(page, picture, area);
        return;
    }
    if (!paper) {
        return;
    }

    if (!m_pdf) {
        m_pdf.emplace();
    }
    const core::ContentId id = asset.id;
    const auto scale = static_cast<float>(columnScale(*paper));
    const auto width = static_cast<int>(
        std::clamp(paper->width * scale, 1.0F, static_cast<float>(kMaximumMediaPixels)));
    const auto height = static_cast<int>(
        std::clamp(paper->height * scale, 1.0F, static_cast<float>(kMaximumMediaPixels)));
    m_drawnAt[page] = scale;
    const QRectF area{0.0, 0.0, static_cast<qreal>(paper->width),
                      static_cast<qreal>(paper->height)};
    const std::uint64_t opening = m_opening;
    m_pdf->open(std::move(asset), [](core::Result<std::vector<platform::pdf::PageSize>>) {});
    m_pdf->render(id, index, width, height,
                  [this, opening, page, area](core::Result<platform::pdf::PageImage> image) {
                      if (!image) {
                          return;
                      }
                      QMetaObject::invokeMethod(
                          this,
                          [this, opening, page, area, drawn = std::move(*image)] {
                              if (opening != m_opening) {
                                  return;
                              }
                              const QImage picture{drawn.pixels.data(), drawn.width, drawn.height,
                                                   static_cast<qsizetype>(drawn.width) * 4,
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
        m_storage->loadPage(page, [this, opening,
                                   page](core::Result<std::vector<core::PlacedStroke>> strokes) {
            QMetaObject::invokeMethod(
                this,
                [this, opening, page, strokes = std::move(strokes)] mutable {
                    m_wantedPages.erase(page);
                    if (opening != m_opening || !strokes || m_pages.contains(page)) {
                        return;
                    }
                    m_pages.emplace(page, std::make_unique<core::Page>(page, std::move(*strokes)));
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

}
