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
#include "core/math/Answer.hpp"
#include "core/math/Equation.hpp"
#include "core/math/Plotting.hpp"
#include "core/math/Reading.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Table.hpp"
#include "core/text/InkWord.hpp"
#include "core/text/WrittenText.hpp"
#include "core/undo/BundleCommand.hpp"
#include "core/undo/LayerCommands.hpp"
#include "core/undo/LinkCommands.hpp"
#include "core/undo/OutlineCommands.hpp"
#include "core/undo/PictureCommands.hpp"
#include "core/undo/StrokeCommands.hpp"
#include "core/undo/TableCommands.hpp"
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
#include <QRectF>
#include <QSizeF>
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

// A name the window hands back, matched against what the page carries: only the page knows which
// thing wears which name, because a name goes out as text and comes back as text.
[[nodiscard]] bool isNamed(const core::Uuid& id, const QString& name) {
    return QString::fromStdString(id.toString()) == name;
}

[[nodiscard]] core::Uuid layerNamed(std::span<const core::Layer> layers, const QString& name) {
    for (const core::Layer& layer : layers) {
        if (isNamed(layer.id, name)) {
            return layer.id;
        }
    }
    return {};
}

constexpr auto kDefaultNotebookName = "default.phvika";
// More than a reader would ever look through at once.
constexpr std::size_t kMostFound = 300;
constexpr auto kKeptPrefix = "kept/";
constexpr int kMaximumMediaPixels = 4096;
constexpr qreal kMediaRedrawFactor = 1.4;
constexpr int kMediaRedrawDelay = 200;
constexpr float kPasteOffset = 24.0F;
constexpr float kPickRadius = 6.0F;
// How large a link stands where there is nothing under it to take its size from.
constexpr float kNewLinkWidth = 80.0F;
constexpr float kNewLinkHeight = 28.0F;
constexpr float kBothSides = 2.0F;
constexpr qreal kTextMargin = 8.0;
// How much of a sheet a picture takes up when it is first put down, and how large a picture is
// kept for drawing, so that a photograph from a modern camera does not ask the graphics card for
// more than it will give.
constexpr qreal kPictureShare = 0.6;
constexpr int kWidestPicture = 4096;
constexpr qreal kNewTextWidth = core::TextBox::kDefaultWidth;
// How much of a sheet a table takes up when it is first ruled, and the way to the middle of what
// is left over.
constexpr qreal kTableShare = 0.8;
constexpr qreal kHalfway = 0.5;
// How far the answer to a sum stands from the writing it answers, and how wide a letter of it is
// reckoned to be, so that the box it goes in holds what it says.
constexpr float kAnswerGap = 10.0F;
constexpr float kLetterWidth = 0.62F;
constexpr std::size_t kRoomAroundAnswer = 2;
// How far in from the edge of the sheet a sum that is typed is put down.
constexpr qreal kEquationInset = 60.0;

// The chain from what was read to what it comes to: put right, read into a structure, worked out.
[[nodiscard]] core::Result<double> workedOut(const std::string& written) {
    const core::Result<core::Equation> equation = core::equationOf(core::tidied(written));
    if (!equation) {
        return std::unexpected{equation.error()};
    }
    return core::answerOf(*equation);
}

// A sum and its answer as one line, with an equals sign between them where the sum has none of
// its own.
[[nodiscard]] std::string saidWith(std::string asked, const std::string& answer) {
    const std::size_t last = asked.find_last_not_of(" \t\n\r");
    const bool hasEquals = last != std::string::npos && asked[last] == '=';
    asked.append(hasEquals ? " " : " = ");
    asked.append(answer);
    return asked;
}

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

NotebookViewModel::NotebookViewModel(QObject* parent)
    : QObject(parent), m_pictureReader{platform::ocr::openReadPicture()} {
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
    forgetLayerPreviews();
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
        auto made = std::make_unique<core::Page>(
            pageId, std::move(loaded->strokes), std::move(loaded->texts),
            std::move(loaded->pictures), std::move(loaded->tables),
            layersOrOne(std::move(loaded->layers)));
        made->setRecordings(std::move(loaded->recordings));
        made->setMarks(std::move(loaded->marks));
        made->setLinks(std::move(loaded->links));
        m_pages.emplace(pageId, std::move(made));
    }
    if (const auto opened = m_pages.find(pageId); opened != m_pages.end()) {
        wantPicturesFor(*opened->second);
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

// The layer one stroke of a page stands on, for the pieces an eraser leaves of it.
[[nodiscard]] core::Uuid layerOfStroke(const core::Page& page, const core::Uuid& strokeId) {
    for (const core::PlacedStroke& placed : page.strokes()) {
        if (placed.stroke.id() == strokeId) {
            return placed.layer;
        }
    }
    return {};
}

// How high a thing stands, so that what is handed to the window is in the order it is drawn.
[[nodiscard]] std::size_t heightOnPage(const core::Page& page, const core::Uuid& stands) {
    const core::Layer* const found = core::layerOf(page.layers(), stands);
    return found == nullptr ? 0 : core::placeOfLayer(page.layers(), found->id);
}

// The boxes of type a page shows, in the order the layers put them. A box on a layer that is not
// shown is left out altogether.
[[nodiscard]] std::vector<core::TextBox> shownBoxesOf(const core::Page& page) {
    std::vector<std::pair<std::size_t, core::TextBox>> standing;
    for (const core::PlacedText& placed : page.texts()) {
        if (core::isShownOn(page.layers(), placed.layer)) {
            standing.emplace_back(heightOnPage(page, placed.layer), placed.box);
        }
    }
    std::ranges::stable_sort(standing, {}, [](const auto& stands) { return stands.first; });
    std::vector<core::TextBox> boxes;
    boxes.reserve(standing.size() + 1);
    for (auto& [height, box] : standing) {
        boxes.push_back(std::move(box));
    }
    return boxes;
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
                                          float radius, core::EraseMode mode, int sheet) {
    m_owner->erase(from, to, radius, mode, sheet);
}

void NotebookViewModel::Sink::selectionDrawn(std::span<const core::Point> shape) {
    m_owner->selectInside(shape);
}

void NotebookViewModel::Sink::selectionMoved(float dx, float dy) {
    m_owner->moveSelection(dx, dy);
}

void NotebookViewModel::Sink::colourSeen(const core::Color& colour) {
    m_owner->showPickedColour(colour);
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
    if (!m_loaded || page == nullptr || !canPutSomethingDown() || !m_storage) {
        refreshCanvas();
        return;
    }
    const core::Result<void> stored = m_history.run(
        std::make_unique<core::AddStrokeCommand>(page, &*m_storage,
                                                 core::PlacedStroke{
                                                     .ordinal = page->nextOrdinal(),
                                                     .stroke = stroke,
                                                     .layer = layerForNewThings(*page),
                                                 }));
    if (!stored) {
        reportError(QString::fromStdString(stored.error().message));
        refreshCanvas();
        return;
    }
    noteTheMoment(stroke.id());
    emit pageChanged();
    publishLayers();
    markEdited();
    emit historyChanged();
}

void NotebookViewModel::showPickedColour(const core::Color& colour) {
    emit colourPicked(QColor::fromRgb(colour.red, colour.green, colour.blue, colour.alpha));
}

void NotebookViewModel::pickColour(const core::InkSample& at, int sheet) {
    const auto found = m_pages.find(pageOfSheet(sheet));
    const core::Page* const page = found == m_pages.end() ? nullptr : found->second.get();
    if (page == nullptr) {
        return;
    }
    if (const core::TextBox* const box = page->textUnder(core::Point{.x = at.x, .y = at.y})) {
        const core::Color& colour = box->style.color;
        emit colourPicked(QColor::fromRgb(colour.red, colour.green, colour.blue, colour.alpha));
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

// Every stroke the sweep reached is set aside to be worked on. A whole line leaves nothing
// behind, so it is gone from the sheet at a touch; a line rubbed in part keeps what the sweeps
// have not reached yet. Says whether the sheet now looks different.
bool NotebookViewModel::noteTouched(const core::Page& page, const core::EraserSweep& sweep,
                                    bool whole) {
    bool changed = false;
    for (const core::Uuid& strokeId : page.strokesTouchedBy(sweep)) {
        if (m_erasePieces.contains(strokeId)) {
            continue;
        }
        const auto found =
            std::ranges::find(page.strokes(), strokeId,
                              [](const core::PlacedStroke& placed) { return placed.stroke.id(); });
        if (found == page.strokes().end()) {
            continue;
        }
        m_erasePieces.emplace(strokeId, whole ? std::vector<core::Stroke>{}
                                              : std::vector<core::Stroke>{found->stroke});
        m_erasing.push_back(strokeId);
        changed = changed || whole;
    }
    return changed;
}

void NotebookViewModel::erase(const core::InkSample& from, const core::InkSample& to, float radius,
                              core::EraseMode mode, int sheet) {
    const core::Uuid on = pageOfSheet(sheet);
    const auto kept = m_pages.find(on);
    const core::Page* const page = kept == m_pages.end() ? nullptr : kept->second.get();
    if (!m_loaded || page == nullptr) {
        return;
    }
    // A sweep stays on the page it started on, even where the eraser runs past its edge.
    m_erasedPage = on;
    const bool whole = mode == core::EraseMode::WholeStroke;
    const core::EraserSweep sweep{
        .from = {.x = from.x, .y = from.y},
        .to = {.x = to.x, .y = to.y},
        .radius = core::reachOf(radius, mode),
    };
    m_sweeps.push_back(sweep);

    bool changed = noteTouched(*page, sweep, whole);

    if (whole) {
        if (changed) {
            refreshCanvas();
        }
        return;
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
            pieces.push_back(core::PlacedStroke{
                .ordinal = ordinal,
                .stroke = piece,
                .layer = layerOfStroke(*page, strokeId),
            });
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
    m_preview.clear();
    m_previewIds.clear();
    if (const core::ICommand* const next = m_history.nextUndo()) {
        const std::optional<core::Uuid> pageToShow = next->pageToShow();
        finishChange(m_history.undo(), pageToShow);
    }
}

void NotebookViewModel::redo() {
    m_erasing.clear();
    m_erasePieces.clear();
    m_sweeps.clear();
    m_preview.clear();
    m_previewIds.clear();
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
        core::PlacedStroke made{
            .ordinal = placed.ordinal,
            .stroke = std::move(fresh),
            .layer = placed.layer,
        };
        std::ignore = page->insert(made);
        copies.push_back(std::move(made));
    }

    std::vector<core::PlacedText> textCopies;
    textCopies.reserve(texts.size());
    for (const core::PlacedText& placed : texts) {
        core::PlacedText made{
            .ordinal = placed.ordinal,
            .box = placed.box,
            .layer = placed.layer,
        };
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
    publishLayers();
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
        .pictures = {},
        .tables = {},
        .layers = {},
        .media = {},
    });
    if (const auto cached = m_pages.find(work->page.id); cached != m_pages.end()) {
        const std::span<const core::PlacedStroke> strokes = cached->second->strokes();
        work->strokes.assign(strokes.begin(), strokes.end());
        const std::span<const core::PlacedText> texts = cached->second->texts();
        work->texts.assign(texts.begin(), texts.end());
        const std::span<const core::PlacedTable> tables = cached->second->tables();
        work->tables.assign(tables.begin(), tables.end());
        const std::span<const core::Layer> layers = cached->second->layers();
        work->layers.assign(layers.begin(), layers.end());
        takePictures(*work, cached->second->pictures());
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
                                    work->tables = std::move(loaded->tables);
                                    work->layers = std::move(loaded->layers);
                                    takePictures(*work, loaded->pictures);
                                    gatherThumbnail(work);
                                },
                                Qt::QueuedConnection);
                        });
}

// Only the pictures this notebook has already read are drawn into a small picture of the page.
// One that arrives later throws that picture away, so the page is drawn again with it.
void NotebookViewModel::takePictures(ThumbnailWork& work,
                                     std::span<const core::PlacedPicture> pictures) const {
    work.pictures.clear();
    work.pictures.reserve(pictures.size());
    for (const core::PlacedPicture& placed : pictures) {
        const auto drawn = m_pictureImages.find(placed.picture.source);
        if (drawn == m_pictureImages.end()) {
            continue;
        }
        work.pictures.push_back(ThumbnailPicture{
            .placed = placed.picture,
            .picture = drawn->second,
            .layer = placed.layer,
        });
    }
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
    std::vector<platform::render::DrawnPicture> pictures;
    pictures.reserve(work.pictures.size());
    for (const ThumbnailPicture& drawn : work.pictures) {
        pictures.push_back(platform::render::DrawnPicture{
            .placed = drawn.placed,
            .picture = &drawn.picture,
            .layer = drawn.layer,
        });
    }
    const platform::render::PageContents contents{
        .style = work.page.style,
        .strokes = work.strokes,
        .texts = work.texts,
        .pictures = pictures,
        .tables = work.tables,
        .layers = work.layers,
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

void NotebookViewModel::wantLayerPreview(const QString& layerId) {
    const core::Page* const page = currentPageData();
    const core::PageInfo* const info = currentPageInfo();
    if (page == nullptr || info == nullptr) {
        return;
    }
    const core::Uuid wanted = layerNamed(page->layers(), layerId);
    if (wanted.isNil() || m_layerPreviews.contains(wanted)) {
        return;
    }
    ThumbnailWork work;
    work.page = *info;
    const std::span<const core::PlacedStroke> strokes = page->strokes();
    work.strokes.assign(strokes.begin(), strokes.end());
    const std::span<const core::PlacedText> texts = page->texts();
    work.texts.assign(texts.begin(), texts.end());
    const std::span<const core::PlacedTable> tables = page->tables();
    work.tables.assign(tables.begin(), tables.end());
    const std::span<const core::Layer> standing = page->layers();
    work.layers.assign(standing.begin(), standing.end());
    takePictures(work, page->pictures());
    paintLayerPreview(work, wanted);
}

void NotebookViewModel::paintLayerPreview(const ThumbnailWork& work, const core::Uuid& layerId) {
    std::vector<platform::render::DrawnPicture> pictures;
    pictures.reserve(work.pictures.size());
    for (const ThumbnailPicture& drawn : work.pictures) {
        pictures.push_back(platform::render::DrawnPicture{
            .placed = drawn.placed,
            .picture = &drawn.picture,
            .layer = drawn.layer,
        });
    }
    std::vector<core::Layer> everything = work.layers;
    for (core::Layer& layer : everything) {
        layer.shown = true;
    }
    std::vector<core::Layer> alone = everything;
    for (core::Layer& layer : alone) {
        layer.shown = layer.id == layerId;
    }
    platform::render::PageContents contents{
        .style = work.page.style,
        .strokes = work.strokes,
        .texts = work.texts,
        .pictures = pictures,
        .tables = work.tables,
        .layers = everything,
        .media = nullptr,
    };
    const core::Rect area = platform::render::pageArea(contents);
    const bool anything = area.width() > 0.0F && area.height() > 0.0F;
    const float ratio = anything ? area.height() / area.width() : kEmptyPageRatio;
    const int height =
        std::max(1, static_cast<int>(static_cast<float>(thumbnails::kWidth) * ratio));
    QImage picture{thumbnails::kWidth, height, QImage::Format_ARGB32_Premultiplied};
    const platform::render::Rgba paper = platform::render::paperColorOf(work.page.style);
    picture.fill(QColor::fromRgbF(paper[0], paper[1], paper[2], paper[3]));
    if (anything) {
        contents.layers = alone;
        QPainter painter{&picture};
        painter.scale(static_cast<double>(thumbnails::kWidth) / static_cast<double>(area.width()),
                      static_cast<double>(height) / static_cast<double>(area.height()));
        platform::render::paintPage(painter, contents, area);
    }

    const int revision = ++m_thumbnailRevision;
    m_layerPreviews[layerId] = revision;
    const QString name = QString::fromStdString(layerId.toString());
    thumbnails::put(QStringLiteral("layer-%1-%2").arg(name).arg(revision), picture);
    m_layersModel.setPreview(name,
                             QStringLiteral("image://pages/layer-%1-%2").arg(name).arg(revision));
}

void NotebookViewModel::forgetLayerPreviews() {
    if (m_layerPreviews.empty()) {
        return;
    }
    for (const auto& [layerId, revision] : m_layerPreviews) {
        const QString name = QString::fromStdString(layerId.toString());
        thumbnails::forget(QStringLiteral("layer-%1").arg(name));
        m_layersModel.setPreview(name, QString{});
    }
    m_layerPreviews.clear();
}

void NotebookViewModel::forgetThumbnail(const core::Uuid& pageId) {
    if (pageId == m_currentPage) {
        forgetLayerPreviews();
    }
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

std::vector<core::Uuid> NotebookViewModel::setAside() const {
    std::vector<core::Uuid> hidden = m_erasing;
    hidden.insert(hidden.end(), m_previewIds.begin(), m_previewIds.end());
    // Ink standing on a layer that is not shown is drawn nowhere, on any page that is open.
    for (const auto& [pageId, page] : m_pages) {
        if (page == nullptr) {
            continue;
        }
        for (const core::PlacedStroke& placed : page->strokes()) {
            if (!core::isShownOn(page->layers(), placed.layer)) {
                hidden.push_back(placed.stroke.id());
            }
        }
    }
    return hidden;
}

std::vector<core::Stroke> NotebookViewModel::standingIn() const {
    std::vector<core::Stroke> shown = m_preview;
    for (const auto& [strokeId, left] : m_erasePieces) {
        shown.insert(shown.end(), left.begin(), left.end());
    }
    return shown;
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
        m_canvas->showPage(*page, style, setAside(), standingIn());
    } else {
        const core::Page placeholder{m_currentPage};
        m_canvas->showPage(placeholder, style);
    }
    if (const auto remembered = m_views.find(m_currentPage); remembered != m_views.end()) {
        m_canvas->showView(remembered->second);
    }
    publishTexts();
    publishTables();
    publishLayers();
    publishRecordings();
    publishLinks();
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

    m_canvas->showColumn(views, currentPage(), setAside(), standingIn());
    publishTexts();
    publishTables();
    publishLayers();
    publishRecordings();
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
                    const auto opened =
                        m_pages
                            .emplace(page,
                                     std::make_unique<core::Page>(
                                         page, std::move(loaded->strokes), std::move(loaded->texts),
                                         std::move(loaded->pictures), std::move(loaded->tables),
                                         layersOrOne(std::move(loaded->layers))))
                            .first;
                    wantPicturesFor(*opened->second);
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

void NotebookViewModel::cutSelection() {
    copySelection();
    deleteSelection();
}

void NotebookViewModel::duplicateSelection() {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
        return;
    }
    const std::vector<core::Uuid>& picked = m_canvas->selection();
    if (picked.empty()) {
        return;
    }
    std::int64_t ordinal = page->nextOrdinal();
    std::vector<core::PlacedStroke> copies;
    std::vector<core::Uuid> ids;
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) == picked.end()) {
            continue;
        }
        const core::Stroke shifted = core::moved(placed.stroke, kPasteOffset, kPasteOffset);
        core::Stroke fresh{m_ids.next(), shifted.style()};
        for (const core::InkSample& sample : shifted.samples()) {
            fresh.append(sample);
        }
        ids.push_back(fresh.id());
        copies.push_back(core::PlacedStroke{
            .ordinal = ordinal,
            .stroke = std::move(fresh),
            .layer = placed.layer,
        });
        ++ordinal;
    }
    if (copies.empty()) {
        return;
    }
    runCommand(std::make_unique<core::AddStrokesCommand>(page, &*m_storage, std::move(copies)));
    m_canvas->showSelection(std::move(ids));
}

std::vector<core::Stroke> NotebookViewModel::pickedStrokes() const {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull()) {
        return {};
    }
    const std::vector<core::Uuid>& picked = m_canvas->selection();
    std::vector<core::Stroke> taken;
    taken.reserve(picked.size());
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) != picked.end()) {
            taken.push_back(placed.stroke);
        }
    }
    return taken;
}

QRectF NotebookViewModel::selectionArea() const {
    std::optional<core::Rect> bounds;
    for (const core::Stroke& stroke : pickedStrokes()) {
        if (const std::optional<core::Rect> box = stroke.boundingBox()) {
            bounds = bounds ? bounds->united(*box) : *box;
        }
    }
    if (!bounds) {
        return {};
    }
    const QRectF where =
        m_canvas.isNull() ? QRectF{} : m_canvas->sheetRect(sheetOfPage(m_currentPage));
    return QRectF{
        QPointF{where.x() + static_cast<qreal>(bounds->left),
                where.y() + static_cast<qreal>(bounds->top)},
        QSizeF{static_cast<qreal>(bounds->width()), static_cast<qreal>(bounds->height())},
    };
}

core::Transform NotebookViewModel::transformOf(const QVariantMap& change) const {
    const QRectF where =
        m_canvas.isNull() ? QRectF{} : m_canvas->sheetRect(sheetOfPage(m_currentPage));
    return core::normalized(core::Transform{
        .pivot =
            core::Point{
                .x = static_cast<float>(change.value("pivotX").toReal() - where.x()),
                .y = static_cast<float>(change.value("pivotY").toReal() - where.y()),
            },
        .dx = static_cast<float>(change.value("dx").toReal()),
        .dy = static_cast<float>(change.value("dy").toReal()),
        .wide = static_cast<float>(change.value("wide", 1.0).toReal()),
        .tall = static_cast<float>(change.value("tall", 1.0).toReal()),
        .turn = static_cast<float>(change.value("turn").toReal()),
    });
}

void NotebookViewModel::showTransform(const QVariantMap& change) {
    const std::vector<core::Stroke> picked = pickedStrokes();
    if (picked.empty() || m_canvas.isNull()) {
        return;
    }
    const core::Transform wanted = transformOf(change);
    m_previewIds = m_canvas->selection();
    m_preview = core::transformed(picked, wanted);
    refreshCanvas();
}

void NotebookViewModel::dropTransform() {
    if (m_preview.empty() && m_previewIds.empty()) {
        return;
    }
    m_preview.clear();
    m_previewIds.clear();
    refreshCanvas();
}

void NotebookViewModel::applyTransform(const QVariantMap& change) {
    core::Page* const page = currentPageData();
    m_preview.clear();
    m_previewIds.clear();
    if (page == nullptr || m_canvas.isNull() || !m_storage) {
        return;
    }
    std::vector<core::Uuid> picked = m_canvas->selection();
    const core::Transform wanted = transformOf(change);
    if (picked.empty() || core::leavesAsItWas(wanted)) {
        refreshCanvas();
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    m_canvas->forgetStrokes(picked);
    runCommand(std::make_unique<core::TransformStrokesCommand>(page, storage, picked, wanted));
    m_canvas->showSelection(std::move(picked));
}

void NotebookViewModel::turnSelection(qreal degrees) {
    const QRectF where = selectionArea();
    if (where.isEmpty()) {
        return;
    }
    applyTransform(QVariantMap{
        {"pivotX", where.center().x()},
        {"pivotY", where.center().y()},
        {"turn", degrees},
    });
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
    if (page == nullptr || m_canvas.isNull() || m_clipboard.empty() || !canPutSomethingDown()
        || !m_storage) {
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
        pasted.push_back(core::PlacedStroke{
            .ordinal = ordinal,
            .stroke = std::move(fresh),
            .layer = layerForNewThings(*page),
        });
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
    // Paper that runs on has no edge to fall inside of, and a tap beside a sheet is still meant
    // for the page being read. Either way the box goes where the reader put it.
    if (m_currentPage.isNil()) {
        return std::nullopt;
    }
    const int sheet = sheetOfPage(m_currentPage);
    const QRectF where = m_canvas->sheetRect(sheet);
    return TextPlace{
        .page = m_currentPage,
        .at =
            core::Point{
                .x = static_cast<float>(column.x() - where.x()),
                .y = static_cast<float>(column.y() - where.y()),
            },
        .sheet = sheet,
    };
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
    map.insert("formula", box.formula);
    if (box.formula) {
        map.insert("drawing", mapOfDrawing(platform::render::drawnFormula(box.text, box.style)));
    }
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
            std::vector<core::TextBox> boxes = shownBoxesOf(*found->second);
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
                    .drawing = box.formula ? mapOfDrawing(platform::render::drawnFormula(box.text,
                                                                                         box.style))
                                           : QVariantMap{},
                    .align = static_cast<int>(box.style.align),
                    .sheet = sheet,
                    .formula = box.formula,
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

// A box holding a sum is exactly as large as what is drawn in it, so that taking hold of it,
// printing it and the small picture of the page all agree about the room it takes.
[[nodiscard]] core::TextBox asLargeAsItDraws(core::TextBox box) {
    if (!box.formula) {
        return box;
    }
    const core::Drawing drawn = platform::render::drawnFormula(box.text, box.style);
    if (drawn.glyphs.empty()) {
        return box;
    }
    box.width = std::max(drawn.width, core::TextBox::kNarrowest);
    box.height = drawn.height;
    return box;
}

void NotebookViewModel::changeText(const core::Uuid& pageId, core::TextBox box) {
    box = asLargeAsItDraws(std::move(box));
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
    runCommand(std::make_unique<core::AddTextCommand>(page, storage,
                                                      core::PlacedText{
                                                          .ordinal = page->nextTextOrdinal(),
                                                          .box = draft.box,
                                                          .layer = layerForNewThings(),
                                                      }));
}

void NotebookViewModel::addTextAt(qreal columnX, qreal columnY, const QVariantMap& style) {
    settleDraft();
    const std::optional<TextPlace> place = placeInColumn(QPointF{columnX, columnY});
    if (!place || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
        publishTexts();
        return;
    }
    const auto found = m_pages.find(place->page);
    if (found == m_pages.end()) {
        publishTexts();
        return;
    }

    const core::TextStyle face = styleOfMap(style);
    const QRectF sheet = m_canvas->sheetRect(place->sheet);
    // Paper without edges gives the box the width it would have anywhere else.
    const qreal room = sheet.width() > 0.0 ? sheet.right() - columnX - kTextMargin : kNewTextWidth;
    core::TextBox box{
        .id = m_ids.next(),
        .at = place->at,
        .width = static_cast<float>(std::clamp(std::min(room, kNewTextWidth),
                                               static_cast<qreal>(core::TextBox::kNarrowest),
                                               kNewTextWidth)),
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

namespace {

// How much of the shorter side of what is being looked at a drawn graph takes.
constexpr float kGraphShare = 0.7F;

// How thick the axes are drawn beside the curve itself.
constexpr float kAxisWidth = 1.0F;
constexpr float kCurveWidth = 2.0F;
constexpr float kRuleWidth = 0.5F;

// How grey the two axes are drawn, and how much paler the rules between them are.
constexpr std::uint8_t kAxisGrey = 128;
constexpr std::uint8_t kRuleGrey = 205;

// A run shorter than this is a single point the eye would never see, so it is left out.
constexpr std::size_t kShortestRun = 2;

[[nodiscard]] double numberIn(const QVariantMap& from, const QString& named, double whenMissing) {
    const auto found = from.find(named);
    return found == from.end() ? whenMissing : found.value().toDouble();
}

// Where a place on the graph falls inside the square the graph is drawn in.
struct Placing {
    double left{};
    double top{};
    double width{};
    double height{};
    core::Frame frame;

    [[nodiscard]] core::InkSample at(double across, double up) const {
        const double wide = core::widthOf(frame);
        const double tall = core::heightOf(frame);
        return core::InkSample{
            .x = static_cast<float>(left + (((across - frame.left) / wide) * width)),
            // A graph counts upwards and a page counts downwards.
            .y = static_cast<float>(top + (((frame.top - up) / tall) * height)),
        };
    }
};

[[nodiscard]] core::Stroke lineOf(const core::Uuid& id, const core::Color& colour, float width,
                                  std::span<const core::InkSample> along) {
    core::Stroke line{id, core::StrokeStyle{.color = colour, .width = width, .roundEnds = true}};
    for (const core::InkSample& sample : along) {
        line.append(sample);
    }
    return line;
}

}

void NotebookViewModel::drawCurve(const QVariantList& runs, const QVariantMap& frame,
                                  const QColor& colour) {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
        return;
    }
    const core::Frame looking{
        .left = numberIn(frame, QStringLiteral("left"), -core::kFrameReach),
        .right = numberIn(frame, QStringLiteral("right"), core::kFrameReach),
        .bottom = numberIn(frame, QStringLiteral("bottom"), -core::kFrameReach),
        .top = numberIn(frame, QStringLiteral("top"), core::kFrameReach),
    };
    if (!core::isDrawable(looking)) {
        return;
    }
    const core::Rect visible = m_canvas->visibleOnPage();
    const float side = std::min(visible.width(), visible.height()) * kGraphShare;
    if (side <= 0.0F) {
        return;
    }
    const Placing placing{
        .left = static_cast<double>(visible.left + ((visible.width() - side) / 2.0F)),
        .top = static_cast<double>(visible.top + ((visible.height() - side) / 2.0F)),
        .width = static_cast<double>(side),
        .height = static_cast<double>(side),
        .frame = looking,
    };

    const core::Color ink = asColor(colour.isValid() ? colour : QColor{Qt::black});
    const core::Color faint{
        .red = kAxisGrey,
        .green = kAxisGrey,
        .blue = kAxisGrey,
        .alpha = core::Color::kOpaque,
    };
    const core::Color paler{
        .red = kRuleGrey,
        .green = kRuleGrey,
        .blue = kRuleGrey,
        .alpha = core::Color::kOpaque,
    };
    const core::Uuid layer = layerForNewThings(*page);
    std::vector<core::PlacedStroke> drawn;
    std::int64_t ordinal = page->nextOrdinal();

    const auto put = [&](const core::Color& with, float width,
                         std::span<const core::InkSample> along) {
        if (along.size() < kShortestRun) {
            return;
        }
        drawn.push_back(core::PlacedStroke{
            .ordinal = ordinal++,
            .stroke = lineOf(m_ids.next(), with, width, along),
            .layer = layer,
        });
    };

    // The rules go down first, so that the axes and the curve stand over them rather than under.
    const double step = core::ruleStep(std::min(core::widthOf(looking), core::heightOf(looking)));
    for (const double at : core::rulesBetween(looking.left, looking.right, step)) {
        const std::array<core::InkSample, 2> rule{
            placing.at(at, looking.bottom),
            placing.at(at, looking.top),
        };
        put(paler, kRuleWidth, rule);
    }
    for (const double at : core::rulesBetween(looking.bottom, looking.top, step)) {
        const std::array<core::InkSample, 2> rule{
            placing.at(looking.left, at),
            placing.at(looking.right, at),
        };
        put(paler, kRuleWidth, rule);
    }

    // The axes are drawn where nothing crosses them at the edge, so a frame that does not hold
    // zero gets its axis along the nearest side rather than none at all.
    const double acrossAt = std::clamp(0.0, looking.bottom, looking.top);
    const double upAt = std::clamp(0.0, looking.left, looking.right);
    const std::array<core::InkSample, 2> sideways{
        placing.at(looking.left, acrossAt),
        placing.at(looking.right, acrossAt),
    };
    const std::array<core::InkSample, 2> upright{
        placing.at(upAt, looking.bottom),
        placing.at(upAt, looking.top),
    };
    put(faint, kAxisWidth, sideways);
    put(faint, kAxisWidth, upright);

    for (const QVariant& one : runs) {
        const QVariantList spots = one.toList();
        std::vector<core::InkSample> along;
        along.reserve(static_cast<std::size_t>(spots.size()));
        for (const QVariant& spot : spots) {
            const QPointF where = spot.toPointF();
            along.push_back(placing.at(where.x(), where.y()));
        }
        put(ink, kCurveWidth, along);
    }
    if (drawn.empty()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::AddStrokesCommand>(page, storage, std::move(drawn)));
    refreshCanvas();
}

void NotebookViewModel::writeDown(const QString& said, const QVariantMap& style, bool formula) {
    const QString words = said.trimmed();
    if (words.isEmpty() || m_canvas.isNull()) {
        return;
    }
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(m_currentPage));
    const core::Rect visible = m_canvas->visibleOnPage();
    addTextAt(sheet.x() + kEquationInset,
              sheet.y() + static_cast<qreal>(visible.top)
                  + (static_cast<qreal>(visible.height()) * kHalfway),
              style);
    const QString textId = m_pickedText;
    if (textId.isEmpty()) {
        return;
    }
    if (formula) {
        markAsFormula(textId);
    }
    finishText(textId, words, 0.0);
}

void NotebookViewModel::writeDownAt(const QString& said, qreal columnX, qreal columnY,
                                    const QVariantMap& style) {
    const QString words = said.trimmed();
    if (words.isEmpty() || m_canvas.isNull()) {
        return;
    }
    addTextAt(columnX, columnY, style);
    const QString textId = m_pickedText;
    if (textId.isEmpty()) {
        return;
    }
    finishText(textId, words, 0.0);
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
    const core::Uuid boxId = box.id;
    changeText(found->first, std::move(box));
    if (draft) {
        settleDraft();
        publishTexts();
    }
    noteTheMoment(boxId);
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

QString NotebookViewModel::wordsOf(const QString& textId) const {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    return found ? QString::fromStdString(found->second.text) : QString{};
}

std::optional<std::pair<core::Uuid, core::Picture>>
NotebookViewModel::pictureById(const QString& pictureId) const {
    for (const auto& [pageId, page] : m_pages) {
        for (const core::PlacedPicture& placed : page->pictures()) {
            if (QString::fromStdString(placed.picture.id.toString()) == pictureId) {
                return std::pair{pageId, placed.picture};
            }
        }
    }
    return std::nullopt;
}

void NotebookViewModel::setPickedPicture(const QString& pictureId) {
    if (pictureId == m_pickedPicture) {
        return;
    }
    m_pickedPicture = pictureId;
    emit pickedPictureChanged();
}

QVariantMap NotebookViewModel::pickedPictureBox() const {
    const std::optional<std::pair<core::Uuid, core::Picture>> found = pictureById(m_pickedPicture);
    if (!found) {
        return {};
    }
    const core::Picture& picture = found->second;
    const QRectF where =
        m_canvas.isNull() ? QRectF{} : m_canvas->sheetRect(sheetOfPage(found->first));
    return QVariantMap{
        {"pictureId", m_pickedPicture},
        {"columnX", where.x() + static_cast<qreal>(picture.at.x)},
        {"columnY", where.y() + static_cast<qreal>(picture.at.y)},
        {"boxWidth", static_cast<qreal>(picture.width)},
        {"boxHeight", static_cast<qreal>(picture.height)},
        {"turn", static_cast<qreal>(picture.turn)},
    };
}

QString NotebookViewModel::pictureUnder(qreal columnX, qreal columnY) const {
    if (m_canvas.isNull()) {
        return {};
    }
    const int sheets = sheetCount();
    for (int sheet = sheets - 1; sheet >= 0; --sheet) {
        const QRectF where = m_canvas->sheetRect(sheet);
        const auto found = m_pages.find(pageOfSheet(sheet));
        if (found == m_pages.end()) {
            continue;
        }
        const core::Point at{
            .x = static_cast<float>(columnX - where.x()),
            .y = static_cast<float>(columnY - where.y()),
        };
        if (const core::Picture* const picture = found->second->pictureUnder(at)) {
            return QString::fromStdString(picture->id.toString());
        }
    }
    return {};
}

void NotebookViewModel::changePicture(const core::Uuid& pageId, core::Picture picture) {
    const auto found = m_pages.find(pageId);
    if (found == m_pages.end() || !m_storage) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(pageId);
    runCommand(std::make_unique<core::ChangePictureCommand>(found->second.get(), storage,
                                                            core::normalized(picture)));
    publishPictures();
    emit pickedPictureChanged();
}

void NotebookViewModel::placePicture(const QString& pictureId, const QVariantMap& where) {
    const std::optional<std::pair<core::Uuid, core::Picture>> found = pictureById(pictureId);
    if (!found || m_canvas.isNull()) {
        return;
    }
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(found->first));
    core::Picture picture = found->second;
    const core::Picture wanted = core::normalized(core::Picture{
        .id = picture.id,
        .source = picture.source,
        .at =
            core::Point{
                .x = static_cast<float>(where.value("columnX").toReal() - sheet.x()),
                .y = static_cast<float>(where.value("columnY").toReal() - sheet.y()),
            },
        .width = static_cast<float>(where.value("boxWidth").toReal()),
        .height = static_cast<float>(where.value("boxHeight").toReal()),
        .turn = static_cast<float>(where.value("turn").toReal()),
    });
    if (wanted == picture) {
        return;
    }
    changePicture(found->first, wanted);
}

void NotebookViewModel::removePicture(const QString& pictureId) {
    const std::optional<std::pair<core::Uuid, core::Picture>> found = pictureById(pictureId);
    if (!found || !m_storage) {
        return;
    }
    const auto page = m_pages.find(found->first);
    if (page == m_pages.end()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    if (pictureId == m_pickedPicture) {
        setPickedPicture({});
    }
    forgetThumbnail(found->first);
    runCommand(std::make_unique<core::RemovePictureCommand>(page->second.get(), storage,
                                                            found->second.id));
    publishPictures();
}

void NotebookViewModel::wantPicturesFor(const core::Page& page) {
    if (!m_storage) {
        return;
    }
    for (const core::PlacedPicture& placed : page.pictures()) {
        const core::ContentId source = placed.picture.source;
        if (m_pictureImages.contains(source) || m_wantedPictures.contains(source)) {
            continue;
        }
        m_wantedPictures.insert(source);
        m_storage->loadAsset(source, [this](core::Result<core::Asset> asset) {
            QMetaObject::invokeMethod(this, [this, asset = std::move(asset)] mutable {
                usePictureAsset(std::move(asset));
            });
        });
    }
    publishPictures();
}

void NotebookViewModel::usePictureAsset(core::Result<core::Asset> asset) {
    if (!asset) {
        reportError(QString::fromStdString(asset.error().message));
        return;
    }
    m_wantedPictures.erase(asset->id);
    QImage picture;
    if (!picture.loadFromData(toByteArray(asset->data))) {
        reportError(tr("A picture on this page could not be read"));
        return;
    }
    // Kept no larger than a graphics card will take, whatever the camera made of it.
    if (picture.width() > kWidestPicture || picture.height() > kWidestPicture) {
        picture = picture.scaled(kWidestPicture, kWidestPicture, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
    }
    const core::ContentId source = asset->id;
    m_pictureImages.insert_or_assign(source, std::move(picture));
    for (const auto& [pageId, page] : m_pages) {
        const bool shows =
            std::ranges::any_of(page->pictures(), [&source](const core::PlacedPicture& placed) {
                return placed.picture.source == source;
            });
        if (shows) {
            forgetThumbnail(pageId);
        }
    }
    publishPictures();
}

void NotebookViewModel::publishPictures() {
    if (m_canvas.isNull()) {
        return;
    }
    std::vector<platform::ink::QtInkItem::PicturePiece> pieces;
    const int sheets = sheetCount();
    for (int sheet = 0; sheet < sheets; ++sheet) {
        const core::Uuid pageId = pageOfSheet(sheet);
        const auto found = m_pages.find(pageId);
        if (found == m_pages.end()) {
            continue;
        }
        // Only the pictures on a layer that is shown, and in the order the layers put them.
        std::vector<std::pair<std::size_t, const core::PlacedPicture*>> standing;
        for (const core::PlacedPicture& placed : found->second->pictures()) {
            if (core::isShownOn(found->second->layers(), placed.layer)) {
                standing.emplace_back(heightOnPage(*found->second, placed.layer), &placed);
            }
        }
        std::ranges::stable_sort(standing, {}, [](const auto& stands) { return stands.first; });
        for (const auto& [height, placed] : standing) {
            const auto drawn = m_pictureImages.find(placed->picture.source);
            if (drawn == m_pictureImages.end()) {
                continue;
            }
            const core::Rect area = core::areaOf(placed->picture);
            pieces.push_back(platform::ink::QtInkItem::PicturePiece{
                .page = pageId,
                .picture = drawn->second,
                .area = QRectF{QPointF{area.left, area.top}, QSizeF{area.width(), area.height()}},
                .turn = placed->picture.turn,
            });
        }
    }
    m_canvas->showPictures(pieces);
}

void NotebookViewModel::solveWhatIsTyped() {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(m_pickedText);
    if (!found) {
        return;
    }
    const core::Result<double> answer = workedOut(found->second.text);
    if (!answer) {
        reportError(QString::fromStdString(answer.error().message));
        return;
    }
    core::TextBox box = found->second;
    box.text = saidWith(box.text, core::writtenAnswer(*answer));
    changeText(found->first, std::move(box));
}

// The answer to a sum, written where it belongs: beside the hand that asked it, at about the size
// that hand wrote in, and with an equals sign in front where the writing has none of its own.
core::TextBox NotebookViewModel::answerBeside(const core::TextBlock& asked,
                                              const std::string& answer, core::TextStyle face) {
    face.size = asked.size;
    const bool hasEquals = asked.text.contains('=');
    const std::string said = hasEquals ? answer : "= " + answer;
    const float wide = static_cast<float>(said.size() + kRoomAroundAnswer)
                       * core::pageUnitsOfPoints(face.size) * kLetterWidth;
    return core::normalized(core::TextBox{
        .id = m_ids.next(),
        .at =
            core::Point{
                .x = asked.area.right + kAnswerGap,
                .y = asked.area.top,
            },
        .width = wide,
        .height = asked.area.height(),
        .text = said,
        .style = std::move(face),
    });
}

void NotebookViewModel::addEquation(const QVariantMap& style) {
    if (m_canvas.isNull()) {
        return;
    }
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(m_currentPage));
    const core::Rect visible = m_canvas->visibleOnPage();
    addTextAt(sheet.x() + kEquationInset,
              sheet.y() + static_cast<qreal>(visible.top)
                  + (static_cast<qreal>(visible.height()) * kHalfway),
              style);
    // What goes in this box is a sum, and is drawn the way arithmetic is written rather than as a
    // plain line of letters.
    markAsFormula(m_pickedText);
}

void NotebookViewModel::markAsFormula(const QString& textId) {
    const std::optional<std::pair<core::Uuid, core::TextBox>> found = textById(textId);
    if (!found || found->second.formula) {
        return;
    }
    core::TextBox box = found->second;
    box.formula = true;
    changeText(found->first, std::move(box));
}

void NotebookViewModel::solveSelection(QVariantMap style) {
    // A sum that is typed is worked out wherever the application runs; one that is written by hand
    // needs a machine that can read handwriting.
    if (!m_pickedText.isEmpty()) {
        solveWhatIsTyped();
        return;
    }
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !m_storage) {
        return;
    }
    const std::vector<core::Uuid> picked = m_canvas->selection();
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
                                           pageId](core::Result<std::vector<core::InkWord>> words) {
        if (!words) {
            reportError(QString::fromStdString(words.error().message));
            return;
        }
        answerWhatWasAsked(pageId, *words, style);
    });
}

void NotebookViewModel::answerWhatWasAsked(const core::Uuid& pageId,
                                           std::span<const core::InkWord> words,
                                           const QVariantMap& style) {
    const std::optional<core::TextBlock> asked = core::textOf(words);
    if (!asked) {
        reportError(tr("Nothing there could be read"));
        return;
    }
    const core::Result<double> answer = workedOut(asked->text);
    if (!answer) {
        reportError(QString::fromStdString(answer.error().message));
        return;
    }

    const auto found = m_pages.find(pageId);
    if (found == m_pages.end() || !m_storage) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    core::Page* const on = found->second.get();
    const core::TextBox box = answerBeside(*asked, core::writtenAnswer(*answer), styleOfMap(style));
    forgetThumbnail(pageId);
    runCommand(std::make_unique<core::AddTextCommand>(on, storage,
                                                      core::PlacedText{
                                                          .ordinal = on->nextTextOrdinal(),
                                                          .box = box,
                                                          .layer = layerForNewThings(),
                                                      }));
    publishTexts();
    setPickedText(QString::fromStdString(box.id.toString()));
}

void NotebookViewModel::addTable(int rows, int columns) {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
        return;
    }

    // As wide as most of the sheet, or of what is on the screen where the paper runs on, every
    // column taking the same share of it.
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(m_currentPage));
    const core::Rect visible = m_canvas->visibleOnPage();
    const qreal room = sheet.width() > 0.0 ? sheet.width() : static_cast<qreal>(visible.width());
    core::Table table = core::gridOf(rows, columns);
    table.id = m_ids.next();
    const float tall = core::heightOf(table);
    const auto wide = static_cast<float>(
        std::max(room * kTableShare, static_cast<qreal>(core::Table::kNarrowestColumn)));
    table = core::sizedTo(std::move(table), wide, tall);
    const float across = core::widthOf(table);
    const float down = core::heightOf(table);
    table.at = core::Point{
        .x = static_cast<float>((room - static_cast<qreal>(across)) * kHalfway),
        .y = visible.top + static_cast<float>((visible.height() - down) * kHalfway),
    };
    table = core::normalized(std::move(table));

    const QString tableId = QString::fromStdString(table.id.toString());
    const core::Uuid madeTable = table.id;
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::AddTableCommand>(page, storage,
                                                       core::PlacedTable{
                                                           .ordinal = page->nextTableOrdinal(),
                                                           .table = std::move(table),
                                                           .layer = layerForNewThings(),
                                                       }));
    noteTheMoment(madeTable);
    publishTables();
    setPickedTable(tableId);
}

std::optional<std::pair<core::Uuid, core::Table>>
NotebookViewModel::tableById(const QString& tableId) const {
    for (const auto& [pageId, page] : m_pages) {
        for (const core::PlacedTable& placed : page->tables()) {
            if (QString::fromStdString(placed.table.id.toString()) == tableId) {
                return std::pair{pageId, placed.table};
            }
        }
    }
    return std::nullopt;
}

void NotebookViewModel::setPickedTable(const QString& tableId) {
    if (tableId == m_pickedTable) {
        return;
    }
    m_pickedTable = tableId;
    emit pickedTableChanged();
}

QVariantMap NotebookViewModel::pickedTableBox() const {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(m_pickedTable);
    if (!found) {
        return {};
    }
    const int sheet = sheetOfPage(found->first);
    const QRectF where = m_canvas.isNull() ? QRectF{} : m_canvas->sheetRect(sheet);
    return mapOfItem(itemOfTable(found->second,
                                 TablePlace{
                                     .pageId = QString::fromStdString(found->first.toString()),
                                     .columnX = where.x() + static_cast<qreal>(found->second.at.x),
                                     .columnY = where.y() + static_cast<qreal>(found->second.at.y),
                                     .sheet = sheet,
                                 }));
}

QString NotebookViewModel::tableUnder(qreal columnX, qreal columnY) const {
    if (m_canvas.isNull()) {
        return {};
    }
    const int sheets = sheetCount();
    for (int sheet = sheets - 1; sheet >= 0; --sheet) {
        const QRectF where = m_canvas->sheetRect(sheet);
        const auto found = m_pages.find(pageOfSheet(sheet));
        if (found == m_pages.end()) {
            continue;
        }
        const core::Point at{
            .x = static_cast<float>(columnX - where.x()),
            .y = static_cast<float>(columnY - where.y()),
        };
        if (const core::Table* const table = found->second->tableUnder(at)) {
            return QString::fromStdString(table->id.toString());
        }
    }
    return {};
}

void NotebookViewModel::changeTable(const core::Uuid& pageId, core::Table table) {
    const auto found = m_pages.find(pageId);
    if (found == m_pages.end() || !m_storage) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(pageId);
    runCommand(std::make_unique<core::ChangeTableCommand>(found->second.get(), storage,
                                                          core::normalized(std::move(table))));
    publishTables();
}

void NotebookViewModel::placeTable(const QString& tableId, const QVariantMap& where) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found || m_canvas.isNull()) {
        return;
    }
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(found->first));
    core::Table wanted =
        core::sizedTo(found->second, static_cast<float>(where.value("boxWidth").toReal()),
                      static_cast<float>(where.value("boxHeight").toReal()));
    wanted.at = core::Point{
        .x = static_cast<float>(where.value("columnX").toReal() - sheet.x()),
        .y = static_cast<float>(where.value("columnY").toReal() - sheet.y()),
    };
    wanted = core::normalized(std::move(wanted));
    if (wanted == found->second) {
        return;
    }
    changeTable(found->first, std::move(wanted));
}

void NotebookViewModel::removeTable(const QString& tableId) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found || !m_storage) {
        return;
    }
    const auto page = m_pages.find(found->first);
    if (page == m_pages.end()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    if (tableId == m_pickedTable) {
        setPickedTable({});
    }
    forgetThumbnail(found->first);
    runCommand(
        std::make_unique<core::RemoveTableCommand>(page->second.get(), storage, found->second.id));
    publishTables();
}

QString NotebookViewModel::wordsOfCell(const QString& tableId, int row, int column) const {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return {};
    }
    const core::TableCell* const cell =
        core::cellAt(found->second, core::CellAt{.row = row, .column = column});
    return cell == nullptr ? QString{} : QString::fromStdString(cell->text);
}

void NotebookViewModel::writeCell(const QString& tableId, int row, int column,
                                  const QString& words) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Result<core::Table> written = core::withCellWritten(
        found->second, core::CellAt{.row = row, .column = column}, words.toStdString());
    if (!written || *written == found->second) {
        return;
    }
    changeTable(found->first, std::move(*written));
}

void NotebookViewModel::reshapeTable(
    const QString& tableId, const std::function<core::Result<core::Table>(core::Table)>& reshaped) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Result<core::Table> wanted = reshaped(found->second);
    if (!wanted) {
        reportError(QString::fromStdString(wanted.error().message));
        return;
    }
    changeTable(found->first, std::move(*wanted));
}

void NotebookViewModel::addRow(const QString& tableId, int at) {
    reshapeTable(tableId,
                 [at](core::Table table) { return core::withRowAdded(std::move(table), at); });
}

void NotebookViewModel::addColumn(const QString& tableId, int at) {
    reshapeTable(tableId,
                 [at](core::Table table) { return core::withColumnAdded(std::move(table), at); });
}

void NotebookViewModel::removeRow(const QString& tableId, int at) {
    reshapeTable(tableId,
                 [at](core::Table table) { return core::withRowRemoved(std::move(table), at); });
}

void NotebookViewModel::removeColumn(const QString& tableId, int at) {
    reshapeTable(tableId,
                 [at](core::Table table) { return core::withColumnRemoved(std::move(table), at); });
}

QVariantMap NotebookViewModel::cellUnder(const QString& tableId, qreal columnX,
                                         qreal columnY) const {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found || m_canvas.isNull()) {
        return {};
    }
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(found->first));
    const std::optional<core::CellAt> cell =
        core::cellUnder(found->second, core::Point{
                                           .x = static_cast<float>(columnX - sheet.x()),
                                           .y = static_cast<float>(columnY - sheet.y()),
                                       });
    if (!cell) {
        return {};
    }
    return QVariantMap{
        {"row", cell->row},
        {"column", cell->column},
    };
}

void NotebookViewModel::spreadTable(const QString& tableId, const QVariantList& widths,
                                    const QVariantList& heights) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    std::vector<float> columns;
    columns.reserve(static_cast<std::size_t>(widths.size()));
    for (const QVariant& width : widths) {
        columns.push_back(static_cast<float>(width.toReal()));
    }
    std::vector<float> rows;
    rows.reserve(static_cast<std::size_t>(heights.size()));
    for (const QVariant& height : heights) {
        rows.push_back(static_cast<float>(height.toReal()));
    }
    core::Table wanted = core::spreadAs(found->second, std::move(columns), std::move(rows));
    if (wanted == found->second) {
        return;
    }
    changeTable(found->first, std::move(wanted));
}

void NotebookViewModel::alignCell(const QString& tableId, int row, int column, int align) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Result<core::Table> aligned = core::withCellAligned(
        found->second, core::CellAt{.row = row, .column = column}, alignOf(align));
    if (!aligned || *aligned == found->second) {
        return;
    }
    changeTable(found->first, std::move(*aligned));
}

void NotebookViewModel::changeRange(
    const QString& tableId, int fromRow, int fromColumn, int toRow, int toColumn,
    const std::function<core::Result<core::Table>(core::Table, core::CellAt, core::CellAt)>&
        change) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Result<core::Table> wanted =
        change(found->second, core::CellAt{.row = fromRow, .column = fromColumn},
               core::CellAt{.row = toRow, .column = toColumn});
    if (!wanted) {
        reportError(QString::fromStdString(wanted.error().message));
        return;
    }
    if (*wanted == found->second) {
        return;
    }
    changeTable(found->first, std::move(*wanted));
}

void NotebookViewModel::alignCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                   int toColumn, int align) {
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [align](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeAligned(std::move(table), from, to, alignOf(align));
                });
}

void NotebookViewModel::riseCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                  int toColumn, int rise) {
    const auto wanted = rise >= 0 && rise <= static_cast<int>(core::CellRise::Bottom)
                            ? static_cast<core::CellRise>(rise)
                            : core::CellRise::Top;
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [wanted](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeRisen(std::move(table), from, to, wanted);
                });
}

void NotebookViewModel::fillCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                  int toColumn, const QColor& fill) {
    const core::Color wanted = fill.isValid() ? asColor(fill) : core::kNoColor;
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [wanted](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeFilled(std::move(table), from, to, wanted);
                });
}

void NotebookViewModel::inkCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                 int toColumn, const QColor& ink) {
    const core::Color wanted = ink.isValid() ? asColor(ink) : core::kNoColor;
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [wanted](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeInked(std::move(table), from, to, wanted);
                });
}

void NotebookViewModel::weighCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                   int toColumn, bool bold) {
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [bold](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeWeighted(std::move(table), from, to, bold);
                });
}

void NotebookViewModel::slantCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                   int toColumn, bool italic) {
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [italic](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeSlanted(std::move(table), from, to, italic);
                });
}

void NotebookViewModel::plainCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                   int toColumn) {
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangePlain(std::move(table), from, to);
                });
}

void NotebookViewModel::emptyCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                   int toColumn) {
    changeRange(tableId, fromRow, fromColumn, toRow, toColumn,
                [](core::Table table, core::CellAt from, core::CellAt to) {
                    return core::withRangeEmptied(std::move(table), from, to);
                });
}

void NotebookViewModel::duplicateRow(const QString& tableId, int at) {
    reshapeTable(tableId,
                 [at](core::Table table) { return core::withRowDuplicated(std::move(table), at); });
}

void NotebookViewModel::duplicateColumn(const QString& tableId, int at) {
    reshapeTable(tableId, [at](core::Table table) {
        return core::withColumnDuplicated(std::move(table), at);
    });
}

void NotebookViewModel::ruleTable(const QString& tableId, const QColor& rule, qreal width) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Table wanted = found->second;
    if (rule.isValid()) {
        wanted.rule = asColor(rule);
    }
    if (width > 0.0) {
        wanted.ruleWidth = static_cast<float>(width);
    }
    wanted = core::normalized(std::move(wanted));
    if (wanted == found->second) {
        return;
    }
    changeTable(found->first, std::move(wanted));
}

QVariantMap NotebookViewModel::cellLook(const QString& tableId, int row, int column) const {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return {};
    }
    const core::TableCell* const cell =
        core::cellAt(found->second, core::CellAt{.row = row, .column = column});
    if (cell == nullptr) {
        return {};
    }
    // A colour a box says nothing about goes out with nothing in it rather than as no colour at
    // all, because a colour the window cannot read is shown as black.
    const auto shown = [](core::Color color) {
        return core::isShown(color) ? asQColor(color) : QColor::fromRgb(0, 0, 0, 0);
    };
    return QVariantMap{
        {"align", static_cast<int>(cell->align)},
        {"rise", static_cast<int>(cell->rise)},
        {"fill", shown(cell->fill)},
        {"ink", shown(cell->ink)},
        {"bold", cell->bold},
        {"italic", cell->italic},
    };
}

void NotebookViewModel::mergeCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                   int toColumn) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Result<core::Table> joined =
        core::withMergedRange(found->second, core::CellAt{.row = fromRow, .column = fromColumn},
                              core::CellAt{.row = toRow, .column = toColumn});
    if (!joined) {
        reportError(QString::fromStdString(joined.error().message));
        return;
    }
    changeTable(found->first, std::move(*joined));
}

void NotebookViewModel::splitCell(const QString& tableId, int row, int column) {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return;
    }
    core::Result<core::Table> apart =
        core::withCellSplit(found->second, core::CellAt{.row = row, .column = column});
    if (!apart) {
        reportError(QString::fromStdString(apart.error().message));
        return;
    }
    changeTable(found->first, std::move(*apart));
}

QVariantMap NotebookViewModel::cellSpan(const QString& tableId, int row, int column) const {
    const std::optional<std::pair<core::Uuid, core::Table>> found = tableById(tableId);
    if (!found) {
        return {};
    }
    const std::optional<core::CellAt> owner =
        core::ownerOf(found->second, core::CellAt{.row = row, .column = column});
    if (!owner) {
        return {};
    }
    const core::TableCell* const cell = core::cellAt(found->second, *owner);
    if (cell == nullptr) {
        return {};
    }
    return QVariantMap{
        {"row", owner->row},
        {"column", owner->column},
        {"across", cell->across},
        {"down", cell->down},
    };
}

namespace {

[[nodiscard]] core::Uuid thingNamed(const core::Page& page, const QString& name) {
    for (const core::PlacedStroke& placed : page.strokes()) {
        if (isNamed(placed.stroke.id(), name)) {
            return placed.stroke.id();
        }
    }
    for (const core::PlacedText& placed : page.texts()) {
        if (isNamed(placed.box.id, name)) {
            return placed.box.id;
        }
    }
    for (const core::PlacedPicture& placed : page.pictures()) {
        if (isNamed(placed.picture.id, name)) {
            return placed.picture.id;
        }
    }
    for (const core::PlacedTable& placed : page.tables()) {
        if (isNamed(placed.table.id, name)) {
            return placed.table.id;
        }
    }
    return {};
}

[[nodiscard]] bool standsOn(const core::Page& page, const core::Uuid& stands,
                            const core::Uuid& layerId) {
    const core::Layer* const found = core::layerOf(page.layers(), stands);
    return found != nullptr && found->id == layerId;
}

[[nodiscard]] core::Stroke copyOf(const core::Stroke& stroke, const core::Uuid& id) {
    core::Stroke made{id, stroke.style()};
    for (const core::InkSample& sample : stroke.samples()) {
        made.append(sample);
    }
    return made;
}

}

std::vector<core::Layer> NotebookViewModel::layersOrOne(std::vector<core::Layer> layers) {
    if (!layers.empty()) {
        return layers;
    }
    return {
        core::Layer{
            .id = m_ids.next(),
            .name = tr("Layer 1").toStdString(),
            .shown = true,
            .locked = false,
        },
    };
}

std::vector<core::Layer> NotebookViewModel::layersHere() const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return {};
    }
    const std::span<const core::Layer> standing = page->layers();
    return {standing.begin(), standing.end()};
}

core::Uuid NotebookViewModel::layerForNewThings() const {
    const core::Page* const page = currentPageData();
    return page == nullptr ? core::Uuid{} : layerForNewThings(*page);
}

core::Uuid NotebookViewModel::layerForNewThings(const core::Page& page) const {
    const std::span<const core::Layer> standing = page.layers();
    for (const core::Layer& layer : standing) {
        if (isNamed(layer.id, m_activeLayer) && core::isOpenToTheHand(layer)) {
            return layer.id;
        }
    }
    // Nothing is ever put on a layer that is hidden or locked: the topmost one that will take it
    // is used instead, which is where a reader would have put it by hand.
    for (std::size_t step = standing.size(); step > 0; --step) {
        if (core::isOpenToTheHand(standing[step - 1])) {
            return standing[step - 1].id;
        }
    }
    return standing.empty() ? core::Uuid{} : standing.back().id;
}

void NotebookViewModel::changeLayers(std::vector<core::Layer> wanted) {
    const core::Page* const page = currentPageData();
    if (page == nullptr || !m_storage || wanted.empty()) {
        return;
    }
    const auto found = m_pages.find(page->id());
    if (found == m_pages.end()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::ChangeLayersCommand>(found->second.get(), storage,
                                                           std::move(wanted)));
    letGoOfWhatIsShut();
    publishLayers();
    refreshCanvas();
}

void NotebookViewModel::letGoOfWhatIsShut() {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return;
    }
    const auto shut = [page](const QString& thingId) {
        return !thingId.isEmpty() && !page->isThingOpenToTheHand(thingNamed(*page, thingId));
    };
    if (shut(m_pickedTable)) {
        setPickedTable({});
    }
    if (shut(m_pickedPicture)) {
        setPickedPicture({});
    }
    if (shut(m_pickedText)) {
        setPickedText({});
    }
    if (m_canvas.isNull()) {
        return;
    }
    const std::vector<core::Uuid> picked = m_canvas->selection();
    std::vector<core::Uuid> kept;
    kept.reserve(picked.size());
    for (const core::Uuid& strokeId : picked) {
        if (page->isThingOpenToTheHand(strokeId)) {
            kept.push_back(strokeId);
        }
    }
    if (kept.size() != picked.size()) {
        m_canvas->showSelection(std::move(kept));
    }
}

bool NotebookViewModel::canPutSomethingDown() {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return true;
    }
    const std::span<const core::Layer> standing = page->layers();
    const core::Layer* const chosen = core::layerOf(standing, layerNamed(standing, m_activeLayer));
    if (chosen == nullptr || core::isOpenToTheHand(*chosen)) {
        return true;
    }
    const QString name = QString::fromStdString(chosen->name);
    reportError(chosen->shown ? tr("“%1” is locked. Unlock it, or work on another layer.").arg(name)
                              : tr("“%1” is hidden. Show it, or work on another layer.").arg(name));
    return false;
}

void NotebookViewModel::setActiveLayer(const QString& layerId) {
    if (layerId == m_activeLayer) {
        return;
    }
    m_activeLayer = layerId;
    emit layersChanged();
}

void NotebookViewModel::addLayer() {
    std::vector<core::Layer> wanted = layersHere();
    if (wanted.empty() || wanted.size() >= core::Layer::kMostLayers) {
        return;
    }
    const std::size_t above = core::placeOfLayer(wanted, layerNamed(wanted, m_activeLayer));
    core::Layer made{
        .id = m_ids.next(),
        .name = core::freeName(wanted, tr("Layer %1").arg(wanted.size() + 1).toStdString()),
        .shown = true,
        .locked = false,
    };
    const QString name = QString::fromStdString(made.id.toString());
    const std::size_t at = above < wanted.size() ? above + 1 : wanted.size();
    wanted.insert(std::next(wanted.begin(), static_cast<std::ptrdiff_t>(at)), std::move(made));
    changeLayers(std::move(wanted));
    setActiveLayer(name);
}

void NotebookViewModel::removeLayer(const QString& layerId) {
    std::vector<core::Layer> wanted = layersHere();
    if (wanted.size() <= 1) {
        reportError(tr("A page keeps at least one layer."));
        return;
    }
    const core::Page* const page = currentPageData();
    const core::Uuid gone = layerNamed(wanted, layerId);
    const std::size_t at = core::placeOfLayer(wanted, gone);
    if (page == nullptr || at >= wanted.size() || !m_storage) {
        return;
    }
    const auto found = m_pages.find(page->id());
    if (found == m_pages.end()) {
        return;
    }
    core::Page* const here = found->second.get();
    core::StorageThread* const storage = &*m_storage;

    // Everything standing on the layer goes with it, and all of it in one step, so that one undo
    // brings the layer and its whole contents back.
    std::vector<std::unique_ptr<core::ICommand>> steps;
    std::vector<core::Uuid> strokes;
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (standsOn(*page, placed.layer, gone)) {
            strokes.push_back(placed.stroke.id());
        }
    }
    if (!strokes.empty()) {
        steps.push_back(
            std::make_unique<core::EraseStrokesCommand>(here, storage, std::move(strokes)));
    }
    for (const core::PlacedText& placed : page->texts()) {
        if (standsOn(*page, placed.layer, gone)) {
            steps.push_back(
                std::make_unique<core::RemoveTextCommand>(here, storage, placed.box.id));
        }
    }
    for (const core::PlacedPicture& placed : page->pictures()) {
        if (standsOn(*page, placed.layer, gone)) {
            steps.push_back(
                std::make_unique<core::RemovePictureCommand>(here, storage, placed.picture.id));
        }
    }
    for (const core::PlacedTable& placed : page->tables()) {
        if (standsOn(*page, placed.layer, gone)) {
            steps.push_back(
                std::make_unique<core::RemoveTableCommand>(here, storage, placed.table.id));
        }
    }
    wanted.erase(std::next(wanted.begin(), static_cast<std::ptrdiff_t>(at)));
    const QString left = QString::fromStdString(wanted.back().id.toString());
    steps.push_back(std::make_unique<core::ChangeLayersCommand>(here, storage, wanted));

    forgetThumbnail(page->id());
    setPickedTable({});
    setPickedPicture({});
    setPickedText({});
    runCommand(std::make_unique<core::BundleCommand>(std::move(steps)));
    setActiveLayer(left);
    publishLayers();
    refreshCanvas();
}

void NotebookViewModel::duplicateLayer(const QString& layerId) {
    std::vector<core::Layer> wanted = layersHere();
    const core::Page* const page = currentPageData();
    const core::Uuid from = layerNamed(wanted, layerId);
    const std::size_t at = core::placeOfLayer(wanted, from);
    if (page == nullptr || at >= wanted.size() || wanted.size() >= core::Layer::kMostLayers
        || !m_storage) {
        return;
    }
    const auto found = m_pages.find(page->id());
    if (found == m_pages.end()) {
        return;
    }
    core::Page* const here = found->second.get();
    core::StorageThread* const storage = &*m_storage;

    core::Layer made{
        .id = m_ids.next(),
        .name = core::freeName(wanted, wanted[at].name),
        .shown = wanted[at].shown,
        .locked = false,
    };
    const core::Uuid onto = made.id;
    const QString name = QString::fromStdString(onto.toString());
    wanted.insert(std::next(wanted.begin(), static_cast<std::ptrdiff_t>(at + 1)), std::move(made));

    // The layer and everything put down again on it go in as one step.
    std::vector<std::unique_ptr<core::ICommand>> steps;
    steps.push_back(std::make_unique<core::ChangeLayersCommand>(here, storage, wanted));

    std::vector<core::PlacedStroke> strokes;
    std::int64_t ordinal = page->nextOrdinal();
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (standsOn(*page, placed.layer, from)) {
            strokes.push_back(core::PlacedStroke{
                .ordinal = ordinal++,
                .stroke = copyOf(placed.stroke, m_ids.next()),
                .layer = onto,
            });
        }
    }
    if (!strokes.empty()) {
        steps.push_back(
            std::make_unique<core::AddStrokesCommand>(here, storage, std::move(strokes)));
    }
    std::int64_t textOrdinal = page->nextTextOrdinal();
    for (const core::PlacedText& placed : page->texts()) {
        if (standsOn(*page, placed.layer, from)) {
            core::TextBox box = placed.box;
            box.id = m_ids.next();
            steps.push_back(std::make_unique<core::AddTextCommand>(
                here, storage,
                core::PlacedText{.ordinal = textOrdinal++, .box = std::move(box), .layer = onto}));
        }
    }
    std::int64_t pictureOrdinal = page->nextPictureOrdinal();
    for (const core::PlacedPicture& placed : page->pictures()) {
        if (standsOn(*page, placed.layer, from)) {
            core::Picture picture = placed.picture;
            picture.id = m_ids.next();
            steps.push_back(
                std::make_unique<core::AddPictureCommand>(here, storage,
                                                          core::PlacedPicture{
                                                              .ordinal = pictureOrdinal++,
                                                              .picture = picture,
                                                              .layer = onto,
                                                          }));
        }
    }
    std::int64_t tableOrdinal = page->nextTableOrdinal();
    for (const core::PlacedTable& placed : page->tables()) {
        if (standsOn(*page, placed.layer, from)) {
            core::Table table = placed.table;
            table.id = m_ids.next();
            steps.push_back(std::make_unique<core::AddTableCommand>(here, storage,
                                                                    core::PlacedTable{
                                                                        .ordinal = tableOrdinal++,
                                                                        .table = std::move(table),
                                                                        .layer = onto,
                                                                    }));
        }
    }

    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::BundleCommand>(std::move(steps)));
    setActiveLayer(name);
    publishLayers();
    refreshCanvas();
}

void NotebookViewModel::renameLayer(const QString& layerId, const QString& name) {
    std::vector<core::Layer> wanted = layersHere();
    const std::size_t at = core::placeOfLayer(wanted, layerNamed(wanted, layerId));
    const std::string asked = name.trimmed().toStdString();
    if (at >= wanted.size() || asked.empty() || wanted[at].name == asked) {
        return;
    }
    const core::Layer standing = wanted[at];
    wanted.erase(std::next(wanted.begin(), static_cast<std::ptrdiff_t>(at)));
    core::Layer renamed = standing;
    renamed.name = core::freeName(wanted, asked);
    wanted.insert(std::next(wanted.begin(), static_cast<std::ptrdiff_t>(at)), std::move(renamed));
    changeLayers(std::move(wanted));
}

void NotebookViewModel::showLayer(const QString& layerId, bool shown) {
    std::vector<core::Layer> wanted = layersHere();
    const std::size_t at = core::placeOfLayer(wanted, layerNamed(wanted, layerId));
    if (at >= wanted.size() || wanted[at].shown == shown) {
        return;
    }
    wanted[at].shown = shown;
    changeLayers(std::move(wanted));
}

void NotebookViewModel::lockLayer(const QString& layerId, bool locked) {
    std::vector<core::Layer> wanted = layersHere();
    const std::size_t at = core::placeOfLayer(wanted, layerNamed(wanted, layerId));
    if (at >= wanted.size() || wanted[at].locked == locked) {
        return;
    }
    wanted[at].locked = locked;
    changeLayers(std::move(wanted));
}

void NotebookViewModel::moveLayer(const QString& layerId, int to) {
    const std::vector<core::Layer> standing = layersHere();
    const std::size_t from = core::placeOfLayer(standing, layerNamed(standing, layerId));
    if (from >= standing.size() || to < 0 || static_cast<std::size_t>(to) >= standing.size()) {
        return;
    }
    std::vector<core::Layer> wanted =
        core::withLayerMoved(standing, from, static_cast<std::size_t>(to));
    if (wanted == standing) {
        return;
    }
    changeLayers(std::move(wanted));
}

void NotebookViewModel::moveToLayer(const QString& thingId, const QString& layerId) {
    const core::Page* const page = currentPageData();
    if (page == nullptr || !m_storage) {
        return;
    }
    const core::Uuid thing = thingNamed(*page, thingId);
    const core::Uuid onto = layerNamed(page->layers(), layerId);
    if (thing.isNil() || onto.isNil()) {
        return;
    }
    const auto found = m_pages.find(page->id());
    if (found == m_pages.end()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    runCommand(
        std::make_unique<core::MoveToLayerCommand>(found->second.get(), storage, thing, onto));
    publishLayers();
    refreshCanvas();
}

std::vector<core::Uuid> NotebookViewModel::whatIsInHand() const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return {};
    }
    std::vector<core::Uuid> held;
    for (const QString& named : {m_pickedTable, m_pickedPicture, m_pickedText}) {
        const core::Uuid thing = named.isEmpty() ? core::Uuid{} : thingNamed(*page, named);
        if (!thing.isNil()) {
            held.push_back(thing);
        }
    }
    if (!m_canvas.isNull()) {
        const std::vector<core::Uuid> picked = m_canvas->selection();
        held.insert(held.end(), picked.begin(), picked.end());
    }
    return held;
}

void NotebookViewModel::movePickedToLayer(const QString& layerId) {
    core::Page* const page = currentPageData();
    if (page == nullptr || !m_storage) {
        return;
    }
    const std::span<const core::Layer> standing = page->layers();
    const core::Layer* const onto = core::layerOf(standing, layerNamed(standing, layerId));
    if (onto == nullptr) {
        return;
    }
    if (!core::isOpenToTheHand(*onto)) {
        const QString name = QString::fromStdString(onto->name);
        reportError(onto->shown ? tr("“%1” is locked and takes nothing.").arg(name)
                                : tr("“%1” is hidden and takes nothing.").arg(name));
        return;
    }
    const std::vector<core::Uuid> held = whatIsInHand();
    const auto found = m_pages.find(page->id());
    if (held.empty() || found == m_pages.end()) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;

    std::vector<std::unique_ptr<core::ICommand>> steps;
    steps.reserve(held.size());
    for (const core::Uuid& thing : held) {
        const std::optional<core::Uuid> stood = page->layerOfThing(thing);
        const core::Layer* const from = stood ? core::layerOf(standing, *stood) : nullptr;
        if (from != nullptr && from->id != onto->id) {
            steps.push_back(std::make_unique<core::MoveToLayerCommand>(found->second.get(), storage,
                                                                       thing, onto->id));
        }
    }
    if (steps.empty()) {
        return;
    }
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::BundleCommand>(std::move(steps)));
    publishLayers();
    refreshCanvas();
}

const core::Recording* NotebookViewModel::recordingNamed(const QString& recordingId) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return nullptr;
    }
    for (const core::Recording& made : page->recordings()) {
        if (isNamed(made.id, recordingId)) {
            return &made;
        }
    }
    return nullptr;
}

void NotebookViewModel::publishRecordings() {
    std::vector<RecordingItem> items;
    std::vector<SayingItem> sayings;
    const core::Page* const page = currentPageData();
    if (page != nullptr) {
        const std::span<const core::Recording> made = page->recordings();
        const std::span<const core::Mark> marks = page->marks();
        items.reserve(made.size());
        for (const core::Recording& recording : made) {
            int tied = 0;
            for (const core::Mark& mark : marks) {
                if (mark.recording == recording.id) {
                    ++tied;
                }
            }
            items.push_back(itemOfRecording(recording, tied));
        }
        if (!m_shownRecording.isEmpty()
            && std::ranges::none_of(made, [this](const core::Recording& recording) {
                   return isNamed(recording.id, m_shownRecording);
               })) {
            m_shownRecording.clear();
        }
        if (m_shownRecording.isEmpty() && !made.empty()) {
            m_shownRecording = QString::fromStdString(made.back().id.toString());
        }
        for (const core::Recording& recording : made) {
            if (!isNamed(recording.id, m_shownRecording)) {
                continue;
            }
            sayings.reserve(recording.said.sayings.size());
            for (const core::Saying& saying : recording.said.sayings) {
                sayings.push_back(SayingItem{
                    .from = saying.from,
                    .to = saying.to,
                    .text = QString::fromStdString(saying.text),
                });
            }
        }
    } else {
        m_shownRecording.clear();
    }
    m_recordingsModel.setItems(std::move(items));
    m_sayingsModel.setItems(std::move(sayings));
    emit recordingsChanged();
}

void NotebookViewModel::showRecording(const QString& recordingId) {
    if (recordingId == m_shownRecording) {
        return;
    }
    m_shownRecording = recordingId;
    publishRecordings();
}

QString NotebookViewModel::beginRecording() {
    core::Page* const page = currentPageData();
    if (page == nullptr) {
        reportError(tr("There is no page to record on."));
        return {};
    }
    core::Recording made;
    made.id = m_ids.next();
    made.madeAt = QDateTime::currentMSecsSinceEpoch();
    made.name = core::plainRecordingName(made.madeAt);
    const core::Uuid recordingId = made.id;
    if (const core::Result<void> put = page->addRecording(std::move(made)); !put) {
        reportError(QString::fromStdString(put.error().message));
        return {};
    }
    m_shownRecording = QString::fromStdString(recordingId.toString());
    publishRecordings();
    return m_shownRecording;
}

int NotebookViewModel::shownReading() const {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_shownRecording.isEmpty()) {
        return static_cast<int>(core::Reading::Unasked);
    }
    for (const core::Recording& made : page->recordings()) {
        if (isNamed(made.id, m_shownRecording)) {
            return static_cast<int>(made.said.reading);
        }
    }
    return static_cast<int>(core::Reading::Unasked);
}

QString NotebookViewModel::shownTrouble() const {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_shownRecording.isEmpty()) {
        return {};
    }
    for (const core::Recording& made : page->recordings()) {
        if (isNamed(made.id, m_shownRecording)) {
            return QString::fromStdString(made.said.trouble);
        }
    }
    return {};
}

void NotebookViewModel::keepRecording(const QString& recordingId, const QByteArray& sound,
                                      qint64 length) {
    core::Page* const page = currentPageData();
    const core::Recording* const made = recordingNamed(recordingId);
    if (page == nullptr || made == nullptr || sound.isEmpty() || !m_storage) {
        giveUpRecording(recordingId);
        reportError(tr("This recording could not be kept."));
        return;
    }
    core::Asset asset{
        .id = hashOf(sound),
        .kind = core::AssetKind::Sound,
        .name = "recording.m4a",
        .data = toBytes(sound),
    };
    const core::ContentId held = asset.id;
    m_storage->submit([asset = std::move(asset)](core::NotebookStore& store) {
        return store.insertAsset(asset);
    });

    core::Recording kept = *made;
    kept.sound = held;
    kept.length = std::max<qint64>(0, length);
    kept = core::normalized(std::move(kept));
    const core::Uuid recordingUuid = kept.id;
    if (const core::Result<void> changed = page->changeRecording(recordingUuid, kept); !changed) {
        return;
    }
    const core::Uuid pageId = page->id();
    m_storage->submit(
        [pageId, kept](core::NotebookStore& store) { return store.insertRecording(pageId, kept); });
    for (const core::Mark& mark : page->marks()) {
        if (mark.recording != recordingUuid) {
            continue;
        }
        m_storage->submit(
            [pageId, mark](core::NotebookStore& store) { return store.markThing(pageId, mark); });
    }
    publishRecordings();
}

void NotebookViewModel::giveUpRecording(const QString& recordingId) {
    core::Page* const page = currentPageData();
    const core::Recording* const made = recordingNamed(recordingId);
    if (page == nullptr || made == nullptr) {
        return;
    }
    const core::Result<core::Recording> taken = page->removeRecording(made->id);
    if (!taken) {
        return;
    }
    if (isNamed(taken->id, m_shownRecording)) {
        m_shownRecording.clear();
    }
    publishRecordings();
}

void NotebookViewModel::renameRecording(const QString& recordingId, const QString& name) {
    core::Page* const page = currentPageData();
    const core::Recording* const made = recordingNamed(recordingId);
    if (page == nullptr || made == nullptr) {
        return;
    }
    const std::string wanted = name.trimmed().toStdString();
    if (wanted.empty() || wanted == made->name) {
        return;
    }
    core::Recording kept = *made;
    kept.name = wanted;
    kept = core::normalized(std::move(kept));
    if (const core::Result<void> changed = page->changeRecording(kept.id, kept); !changed) {
        return;
    }
    if (m_storage) {
        m_storage->submit(
            [kept](core::NotebookStore& store) { return store.updateRecording(kept); });
    }
    publishRecordings();
}

void NotebookViewModel::removeRecording(const QString& recordingId) {
    core::Page* const page = currentPageData();
    const core::Recording* const made = recordingNamed(recordingId);
    if (page == nullptr || made == nullptr) {
        return;
    }
    const core::Uuid gone = made->id;
    if (const core::Result<core::Recording> taken = page->removeRecording(gone); !taken) {
        reportError(QString::fromStdString(taken.error().message));
        return;
    }
    if (m_storage) {
        m_storage->submit(
            [gone](core::NotebookStore& store) { return store.removeRecording(gone); });
    }
    if (isNamed(gone, m_shownRecording)) {
        m_shownRecording.clear();
    }
    publishRecordings();
}

void NotebookViewModel::wantSound(const QString& recordingId) {
    const core::Recording* const made = recordingNamed(recordingId);
    if (made == nullptr || !m_storage) {
        emit soundMissing(recordingId);
        return;
    }
    const std::uint64_t opening = m_opening;
    const core::ContentId held = made->sound;
    m_storage->loadAsset(held, [this, opening, recordingId](core::Result<core::Asset> asset) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, recordingId, asset = std::move(asset)] {
                if (opening != m_opening) {
                    return;
                }
                if (!asset) {
                    emit soundMissing(recordingId);
                    return;
                }
                QByteArray sound;
                sound.resize(static_cast<qsizetype>(asset->data.size()));
                for (std::size_t step = 0; step < asset->data.size(); ++step) {
                    sound[static_cast<qsizetype>(step)] = static_cast<char>(asset->data[step]);
                }
                emit soundReady(recordingId, sound);
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::markFrom(const QString& recordingId, qint64 at) {
    const core::Recording* const made = recordingNamed(recordingId);
    m_markingInto = made == nullptr ? core::Uuid{} : made->id;
    m_markingAt = std::max<qint64>(0, at);
}

void NotebookViewModel::noteTheMoment(const core::Uuid& thing) {
    if (m_markingInto.isNil() || thing.isNil()) {
        return;
    }
    core::Page* const page = currentPageData();
    if (page == nullptr) {
        return;
    }
    const core::Mark mark{.recording = m_markingInto, .thing = thing, .at = m_markingAt};
    if (const core::Result<void> tied = page->addMark(mark); !tied) {
        return;
    }
    if (m_storage) {
        const core::Uuid pageId = page->id();
        m_storage->submit(
            [pageId, mark](core::NotebookStore& store) { return store.markThing(pageId, mark); });
    }
    publishRecordings();
}

qint64 NotebookViewModel::momentOf(const QString& thingId) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return -1;
    }
    for (const core::Mark& mark : page->marks()) {
        if (isNamed(mark.thing, thingId)) {
            return mark.at;
        }
    }
    return -1;
}

QString NotebookViewModel::recordingOf(const QString& thingId) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return {};
    }
    for (const core::Mark& mark : page->marks()) {
        if (isNamed(mark.thing, thingId)) {
            return QString::fromStdString(mark.recording.toString());
        }
    }
    return {};
}

QString NotebookViewModel::thingWrittenAt(const QString& recordingId, qint64 at) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return {};
    }
    for (const core::Recording& made : page->recordings()) {
        if (!isNamed(made.id, recordingId)) {
            continue;
        }
        const core::Mark* const mark = core::markAt(page->marks(), made.id, at);
        return mark == nullptr ? QString{} : QString::fromStdString(mark->thing.toString());
    }
    return {};
}

void NotebookViewModel::keepSayings(const QString& recordingId, const QVariantList& sayings,
                                    const QString& language) {
    core::Page* const page = currentPageData();
    const core::Recording* const made = recordingNamed(recordingId);
    if (page == nullptr || made == nullptr) {
        return;
    }
    std::vector<core::Saying> runs;
    runs.reserve(static_cast<std::size_t>(sayings.size()));
    for (const QVariant& held : sayings) {
        const QVariantMap said = held.toMap();
        const QString words = said.value(QStringLiteral("text")).toString().trimmed();
        if (words.isEmpty()) {
            continue;
        }
        runs.push_back(core::Saying{
            .from = std::max<qint64>(0, said.value(QStringLiteral("from")).toLongLong()),
            .to = std::max<qint64>(0, said.value(QStringLiteral("to")).toLongLong()),
            .text = words.toStdString(),
        });
    }
    core::Said said;
    said.sayings = runs;
    said.reading = runs.empty() ? core::Reading::Failed : core::Reading::Read;
    said.language = language.toStdString();
    said.trouble = runs.empty() ? "nothing could be made out" : std::string{};
    const core::Uuid recordingUuid = made->id;
    if (const core::Result<void> kept = page->setSaid(recordingUuid, std::move(said)); !kept) {
        return;
    }
    const core::Recording* const now = recordingNamed(recordingId);
    if (m_storage && now != nullptr) {
        m_storage->submit([kept = *now, runs](core::NotebookStore& store) {
            if (const core::Result<void> written = store.writeSayings(kept.id, runs); !written) {
                return written;
            }
            return store.updateRecording(kept);
        });
    }
    publishRecordings();
}

void NotebookViewModel::markReading(const QString& recordingId, int reading,
                                    const QString& trouble) {
    core::Page* const page = currentPageData();
    const core::Recording* const made = recordingNamed(recordingId);
    if (page == nullptr || made == nullptr) {
        return;
    }
    core::Said said = made->said;
    said.reading = reading >= 0 && reading <= static_cast<int>(core::Reading::Failed)
                       ? static_cast<core::Reading>(reading)
                       : core::Reading::Unasked;
    said.trouble = trouble.toStdString();
    const core::Uuid recordingUuid = made->id;
    if (const core::Result<void> kept = page->setSaid(recordingUuid, std::move(said)); !kept) {
        return;
    }
    const core::Recording* const now = recordingNamed(recordingId);
    if (m_storage && now != nullptr) {
        m_storage->submit(
            [kept = *now](core::NotebookStore& store) { return store.updateRecording(kept); });
    }
    publishRecordings();
}

const core::Link* NotebookViewModel::linkNamed(const QString& linkId) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return nullptr;
    }
    for (const core::PlacedLink& placed : page->links()) {
        if (isNamed(placed.link.id, linkId)) {
            return &placed.link;
        }
    }
    return nullptr;
}

QVariantList NotebookViewModel::links() const {
    QVariantList shown;
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return shown;
    }
    for (const core::PlacedLink& placed : page->links()) {
        const core::Link& link = placed.link;
        shown.append(QVariantMap{
            {QStringLiteral("linkId"), QString::fromStdString(link.id.toString())},
            {QStringLiteral("columnX"), static_cast<qreal>(link.at.x)},
            {QStringLiteral("columnY"), static_cast<qreal>(link.at.y)},
            {QStringLiteral("width"), static_cast<qreal>(link.width)},
            {QStringLiteral("height"), static_cast<qreal>(link.height)},
            {QStringLiteral("toPage"), link.kind == core::LinkKind::Page},
            {
                QStringLiteral("where"),
                link.kind == core::LinkKind::Page ? QString::fromStdString(link.page.toString())
                                                  : QString::fromStdString(link.where),
            },
            {QStringLiteral("label"), QString::fromStdString(link.label)},
            {QStringLiteral("reachable"), core::goesSomewhere(link)},
            {QStringLiteral("open"), core::isOpenToTheHand(page->layers(), placed.layer)},
        });
    }
    return shown;
}

QVariantList NotebookViewModel::pagesToLinkTo() const {
    QVariantList shown;
    const std::span<const core::SectionInfo> sections = m_outline.sections();
    for (std::size_t at = 0; at < sections.size(); ++at) {
        const core::SectionInfo& section = sections[at];
        for (std::size_t step = 0; step < section.pages.size(); ++step) {
            const core::PageInfo& page = section.pages[step];
            shown.append(QVariantMap{
                {QStringLiteral("pageId"), QString::fromStdString(page.id.toString())},
                {
                    QStringLiteral("title"),
                    page.title.empty() ? tr("Page %1").arg(step + 1)
                                       : QString::fromStdString(page.title),
                },
                {
                    QStringLiteral("section"),
                    section.title.empty() ? tr("Section %1").arg(at + 1)
                                          : QString::fromStdString(section.title),
                },
                {QStringLiteral("here"), page.id == m_currentPage},
            });
        }
    }
    return shown;
}

void NotebookViewModel::publishLinks() {
    emit linksChanged();
}

namespace {

[[nodiscard]] QVariantMap asArea(const core::Rect& area) {
    return QVariantMap{
        {QStringLiteral("columnX"), static_cast<qreal>(area.left)},
        {QStringLiteral("columnY"), static_cast<qreal>(area.top)},
        {QStringLiteral("width"), static_cast<qreal>(area.width())},
        {QStringLiteral("height"), static_cast<qreal>(area.height())},
    };
}

}

std::optional<core::Rect> NotebookViewModel::areaOfPickedInk(const core::Page& page) const {
    if (m_canvas.isNull() || m_canvas->selectedCount() == 0) {
        return std::nullopt;
    }
    const std::vector<core::Uuid> picked = m_canvas->selection();
    std::optional<core::Rect> around;
    for (const core::PlacedStroke& placed : page.strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) == picked.end()) {
            continue;
        }
        if (const std::optional<core::Rect> bounds = placed.stroke.boundingBox()) {
            around = around ? around->united(*bounds) : *bounds;
        }
    }
    return around;
}

std::optional<core::Rect> NotebookViewModel::areaOfPickedThing(const core::Page& page) const {
    for (const core::PlacedText& placed : page.texts()) {
        if (!m_pickedText.isEmpty() && isNamed(placed.box.id, m_pickedText)) {
            return core::areaOf(placed.box);
        }
    }
    for (const core::PlacedPicture& placed : page.pictures()) {
        if (!m_pickedPicture.isEmpty() && isNamed(placed.picture.id, m_pickedPicture)) {
            return core::areaOf(placed.picture);
        }
    }
    for (const core::PlacedTable& placed : page.tables()) {
        if (!m_pickedTable.isEmpty() && isNamed(placed.table.id, m_pickedTable)) {
            return core::areaOf(placed.table);
        }
    }
    return std::nullopt;
}

QVariantMap NotebookViewModel::areaOfWhatIsPicked() const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return {};
    }
    if (const std::optional<core::Rect> around = areaOfPickedThing(*page)) {
        return asArea(*around);
    }
    if (const std::optional<core::Rect> around = areaOfPickedInk(*page)) {
        return asArea(*around);
    }
    return {};
}

NotebookViewModel::Handful NotebookViewModel::handfulPicked() const {
    Handful handful;
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return handful;
    }
    const std::vector<core::Uuid> picked =
        m_canvas.isNull() ? std::vector<core::Uuid>{} : m_canvas->selection();
    for (const core::PlacedStroke& placed : page->strokes()) {
        if (std::ranges::find(picked, placed.stroke.id()) != picked.end()) {
            handful.strokes.push_back(placed);
        }
    }
    for (const core::PlacedText& placed : page->texts()) {
        if (!m_pickedText.isEmpty() && isNamed(placed.box.id, m_pickedText)) {
            handful.texts.push_back(placed);
        }
    }
    for (const core::PlacedTable& placed : page->tables()) {
        if (!m_pickedTable.isEmpty() && isNamed(placed.table.id, m_pickedTable)) {
            handful.tables.push_back(placed);
        }
    }
    for (const core::PlacedPicture& placed : page->pictures()) {
        if (!m_pickedPicture.isEmpty() && isNamed(placed.picture.id, m_pickedPicture)) {
            handful.pictures.push_back(Handful::Carried{.placed = placed, .bytes = nullptr});
        }
    }

    std::optional<core::Rect> around;
    const auto widen = [&around](const core::Rect& one) {
        around = around ? around->united(one) : one;
    };
    for (const core::PlacedStroke& placed : handful.strokes) {
        if (const std::optional<core::Rect> bounds = placed.stroke.boundingBox()) {
            widen(*bounds);
        }
    }
    for (const core::PlacedText& placed : handful.texts) {
        widen(core::areaOf(placed.box));
    }
    for (const core::PlacedTable& placed : handful.tables) {
        widen(core::areaOf(placed.table));
    }
    for (const Handful::Carried& carried : handful.pictures) {
        widen(core::areaOf(carried.placed.picture));
    }
    handful.area = around.value_or(core::Rect{});
    return handful;
}

void NotebookViewModel::takeHandful(const HandfulReady& ready) {
    Handful handful = handfulPicked();
    if (handful.pictures.empty() || !m_storage) {
        ready(std::move(handful));
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    const std::uint64_t opening = m_opening;
    // The handful is held by a pointer so that every picture can be filled in as it comes back,
    // and handed on once the last one has.
    const auto held = std::make_shared<Handful>(std::move(handful));
    const auto waiting = std::make_shared<std::size_t>(held->pictures.size());
    for (std::size_t step = 0; step < held->pictures.size(); ++step) {
        storage->loadAsset(
            held->pictures[step].placed.picture.source,
            [this, opening, held, waiting, step, ready](core::Result<core::Asset> asset) {
                QMetaObject::invokeMethod(
                    this,
                    [this, opening, held, waiting, step, ready, asset = std::move(asset)] mutable {
                        if (opening != m_opening) {
                            return;
                        }
                        if (asset) {
                            held->pictures[step].bytes =
                                std::make_shared<const std::vector<std::byte>>(
                                    std::move(asset->data));
                        }
                        *waiting -= 1;
                        if (*waiting == 0) {
                            ready(*held);
                        }
                    },
                    Qt::QueuedConnection);
            });
    }
}

void NotebookViewModel::putDownHandful(const Handful& handful) {
    putHandful(handful, std::nullopt);
}

void NotebookViewModel::putDownHandfulAt(const Handful& handful, qreal columnX, qreal columnY) {
    const std::optional<TextPlace> place = placeInColumn(QPointF{columnX, columnY});
    putHandful(handful, place ? std::optional<core::Point>{place->at} : std::nullopt);
}

void NotebookViewModel::putHandful(const Handful& handful,
                                   const std::optional<core::Point>& corner) {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
        return;
    }
    if (handful.strokes.empty() && handful.texts.empty() && handful.tables.empty()
        && handful.pictures.empty()) {
        return;
    }
    const core::Rect visible = m_canvas->visibleOnPage();
    const auto halfway = static_cast<float>(kHalfway);
    const float across = corner
                             ? corner->x - handful.area.left
                             : visible.left + ((visible.width() - handful.area.width()) * halfway)
                                   - handful.area.left;
    const float down = corner ? corner->y - handful.area.top
                              : visible.top + ((visible.height() - handful.area.height()) * halfway)
                                    - handful.area.top;
    const core::Uuid layer = layerForNewThings(*page);
    core::StorageThread* const storage = &*m_storage;

    std::vector<std::unique_ptr<core::ICommand>> steps;
    std::int64_t ordinal = page->nextOrdinal();
    for (const core::PlacedStroke& placed : handful.strokes) {
        const core::Stroke shifted = core::moved(placed.stroke, across, down);
        core::Stroke fresh{m_ids.next(), shifted.style()};
        for (const core::InkSample& sample : shifted.samples()) {
            fresh.append(sample);
        }
        steps.push_back(std::make_unique<core::AddStrokeCommand>(page, storage,
                                                                 core::PlacedStroke{
                                                                     .ordinal = ordinal++,
                                                                     .stroke = std::move(fresh),
                                                                     .layer = layer,
                                                                 }));
    }
    std::int64_t textOrdinal = page->nextTextOrdinal();
    for (const core::PlacedText& placed : handful.texts) {
        core::TextBox box = placed.box;
        box.id = m_ids.next();
        box.at.x += across;
        box.at.y += down;
        steps.push_back(std::make_unique<core::AddTextCommand>(page, storage,
                                                               core::PlacedText{
                                                                   .ordinal = textOrdinal++,
                                                                   .box = core::normalized(box),
                                                                   .layer = layer,
                                                               }));
    }
    std::int64_t tableOrdinal = page->nextTableOrdinal();
    for (const core::PlacedTable& placed : handful.tables) {
        core::Table table = placed.table;
        table.id = m_ids.next();
        table.at.x += across;
        table.at.y += down;
        steps.push_back(
            std::make_unique<core::AddTableCommand>(page, storage,
                                                    core::PlacedTable{
                                                        .ordinal = tableOrdinal++,
                                                        .table = core::normalized(table),
                                                        .layer = layer,
                                                    }));
    }
    std::int64_t pictureOrdinal = page->nextPictureOrdinal();
    for (const Handful::Carried& carried : handful.pictures) {
        if (carried.bytes == nullptr || carried.bytes->empty()) {
            continue;
        }
        core::Picture picture = carried.placed.picture;
        picture.id = m_ids.next();
        picture.at.x += across;
        picture.at.y += down;
        // What the picture is made of is written first, so that the row pointing at it never
        // stands for a moment with nothing behind it.
        const auto bytes = carried.bytes;
        const core::ContentId source = picture.source;
        storage->submit([source, bytes](core::NotebookStore& store) {
            return store.insertAsset(core::Asset{
                .id = source,
                .kind = core::AssetKind::Image,
                .name = {},
                .data = *bytes,
            });
        });
        steps.push_back(
            std::make_unique<core::AddPictureCommand>(page, storage,
                                                      core::PlacedPicture{
                                                          .ordinal = pictureOrdinal++,
                                                          .picture = core::normalized(picture),
                                                          .layer = layer,
                                                      }));
    }
    if (steps.empty()) {
        return;
    }
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::BundleCommand>(std::move(steps)));
    publishTexts();
    publishTables();
    refreshCanvas();
}

bool NotebookViewModel::canReadPictures() const {
    return m_pictureReader != nullptr;
}

const core::Picture* NotebookViewModel::pictureNamed(const QString& pictureId) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return nullptr;
    }
    for (const core::PlacedPicture& placed : page->pictures()) {
        if (isNamed(placed.picture.id, pictureId)) {
            return &placed.picture;
        }
    }
    return nullptr;
}

QString NotebookViewModel::wordsInPicture(const QString& pictureId) const {
    const core::Picture* const standing = pictureNamed(pictureId);
    if (standing == nullptr) {
        return {};
    }
    const auto found = m_pictureWords.find(standing->source);
    return found == m_pictureWords.end() ? QString{} : found->second;
}

QVariantList NotebookViewModel::wordsFoundInPicture(const QString& pictureId) const {
    const core::Picture* const standing = pictureNamed(pictureId);
    if (standing == nullptr) {
        return {};
    }
    const auto found = m_pictureFound.find(standing->source);
    if (found == m_pictureFound.end()) {
        return {};
    }
    QVariantList shown;
    shown.reserve(static_cast<qsizetype>(found->second.size()));
    for (const core::PictureWord& word : found->second) {
        shown.append(QVariantMap{
            {QStringLiteral("text"), QString::fromStdString(word.text)},
            {QStringLiteral("left"), word.box.left},
            {QStringLiteral("top"), word.box.top},
            {QStringLiteral("right"), word.box.right},
            {QStringLiteral("bottom"), word.box.bottom},
        });
    }
    return shown;
}

void NotebookViewModel::forgetWordsInPicture(const QString& pictureId) {
    const core::Picture* const standing = pictureNamed(pictureId);
    if (standing == nullptr) {
        return;
    }
    const core::ContentId source = standing->source;
    m_pictureWords.erase(source);
    m_pictureFound.erase(source);
    if (!m_storage) {
        return;
    }
    m_storage->submit([source](core::NotebookStore& store) -> core::Result<void> {
        return store.forgetPictureWords(source);
    });
}

void NotebookViewModel::keepWhatIsInPicture(const core::ContentId& source,
                                            std::span<const platform::ocr::Found> found) {
    // Held by a pointer the writer can take along without any chance of failing.
    auto words = std::make_shared<std::vector<core::PictureWord>>();
    words->reserve(found.size());
    for (const platform::ocr::Found& one : found) {
        words->push_back(core::PictureWord{
            .text = one.text,
            .box =
                core::Rect{
                    .left = one.left,
                    .top = one.top,
                    .right = one.right,
                    .bottom = one.bottom,
                },
        });
    }
    if (!m_storage) {
        return;
    }
    m_pictureFound[source] = *words;
    m_storage->submit(
        [source, words = std::shared_ptr<const std::vector<core::PictureWord>>{std::move(words)}](
            core::NotebookStore& store) -> core::Result<void> {
            return store.writePictureWords(source, *words);
        });
}

void NotebookViewModel::hearWhatIsInPicture(const QString& pictureId, const core::ContentId& source,
                                            std::uint64_t opening,
                                            core::Result<std::vector<platform::ocr::Found>> found) {
    if (opening != m_opening) {
        return;
    }
    if (!found) {
        emit pictureUnread(pictureId, QString::fromStdString(found.error().message));
        return;
    }
    QString words;
    for (const platform::ocr::Found& one : *found) {
        if (!words.isEmpty()) {
            words += QLatin1Char{'\n'};
        }
        words += QString::fromStdString(one.text);
    }
    if (words.isEmpty()) {
        emit pictureUnread(pictureId, tr("There are no words to be found in this picture."));
        return;
    }
    m_pictureWords[source] = words;
    keepWhatIsInPicture(source, *found);
    emit pictureRead(pictureId, words);
}

void NotebookViewModel::readWhatIsInPicture(const QString& pictureId, const core::ContentId& source,
                                            std::uint64_t opening, const QString& language,
                                            core::Result<core::Asset> asset) {
    if (opening != m_opening) {
        return;
    }
    if (!asset) {
        emit pictureUnread(pictureId, QString::fromStdString(asset.error().message));
        return;
    }
    if (m_pictureReader == nullptr) {
        emit pictureUnread(pictureId, tr("This machine cannot read the words in a picture."));
        return;
    }
    // Held by a pointer so that the bytes outlive the asking, however long the reader takes.
    const auto bytes = std::make_shared<const std::vector<std::byte>>(std::move(asset->data));
    m_pictureReader->read(*bytes, language.toStdString(),
                          [this, opening, pictureId, source,
                           bytes](core::Result<std::vector<platform::ocr::Found>> found) {
                              hearWhatIsInPicture(pictureId, source, opening, std::move(found));
                          });
}

void NotebookViewModel::askTheReaderAboutPicture(const QString& pictureId,
                                                 const core::ContentId& source,
                                                 const QString& language) {
    if (m_pictureReader == nullptr) {
        emit pictureUnread(pictureId, tr("This machine cannot read the words in a picture."));
        return;
    }
    if (!m_storage) {
        emit pictureUnread(pictureId, tr("This picture could not be read from the notebook."));
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    const std::uint64_t opening = m_opening;
    emit readingPicture(pictureId);
    storage->loadAsset(
        source, [this, opening, pictureId, source, language](core::Result<core::Asset> asset) {
            QMetaObject::invokeMethod(
                this,
                [this, opening, pictureId, source, language, asset = std::move(asset)] mutable {
                    readWhatIsInPicture(pictureId, source, opening, language, std::move(asset));
                },
                Qt::QueuedConnection);
        });
}

void NotebookViewModel::readPicture(const QString& pictureId, const QString& language) {
    const core::Picture* const standing = pictureNamed(pictureId);
    if (standing == nullptr) {
        emit pictureUnread(pictureId, tr("This picture is not on the page."));
        return;
    }
    const core::ContentId source = standing->source;
    if (const auto already = m_pictureWords.find(source);
        already != m_pictureWords.end() && !already->second.isEmpty()) {
        emit pictureRead(pictureId, already->second);
        return;
    }
    if (!m_storage) {
        emit pictureUnread(pictureId, tr("This picture could not be read from the notebook."));
        return;
    }
    // What was read before is in the notebook, so a picture read in an earlier sitting comes back
    // at once, and comes back on a machine that carries no reader of its own.
    core::StorageThread* const storage = &*m_storage;
    const std::uint64_t opening = m_opening;
    emit readingPicture(pictureId);
    storage->submit([this, opening, pictureId, source,
                     language](core::NotebookStore& store) -> core::Result<void> {
        core::Result<std::vector<core::PictureWord>> kept = store.pictureWords(source);
        if (!kept) {
            return std::unexpected{kept.error()};
        }
        QMetaObject::invokeMethod(
            this,
            [this, opening, pictureId, source, language, kept = std::move(*kept)] {
                if (opening != m_opening) {
                    return;
                }
                if (kept.empty()) {
                    askTheReaderAboutPicture(pictureId, source, language);
                    return;
                }
                QString words;
                for (const core::PictureWord& word : kept) {
                    if (!words.isEmpty()) {
                        words += QLatin1Char{'\n'};
                    }
                    words += QString::fromStdString(word.text);
                }
                m_pictureWords[source] = words;
                m_pictureFound[source] = kept;
                emit pictureRead(pictureId, words);
            },
            Qt::QueuedConnection);
        return {};
    });
}

void NotebookViewModel::readEveryPicture(const QString& language) {
    if (m_pictureReader == nullptr || !m_pictureQueue.empty() || !m_storage) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    const std::uint64_t opening = m_opening;
    storage->submit([this, opening, language](core::NotebookStore& store) -> core::Result<void> {
        core::Result<std::vector<core::ContentId>> waiting = store.picturesWaitingToBeRead();
        if (!waiting) {
            return std::unexpected{waiting.error()};
        }
        QMetaObject::invokeMethod(
            this,
            [this, opening, language, waiting = std::move(*waiting)] {
                if (opening != m_opening) {
                    return;
                }
                m_pictureQueue.clear();
                for (const core::ContentId& source : waiting) {
                    // A picture already read in this sitting is not read again, even where nothing
                    // was found in it.
                    if (!m_pictureWords.contains(source)) {
                        m_pictureQueue.push_back(source);
                    }
                }
                emit readingPicturesChanged();
                readTheNextPicture(language);
            },
            Qt::QueuedConnection);
        return {};
    });
}

void NotebookViewModel::giveUpReadingPictures() {
    if (m_pictureQueue.empty()) {
        return;
    }
    m_pictureQueue.clear();
    if (m_pictureReader != nullptr) {
        m_pictureReader->giveUp();
    }
    emit readingPicturesChanged();
}

void NotebookViewModel::readTheNextPicture(const QString& language) {
    if (m_pictureQueue.empty() || m_pictureReader == nullptr || !m_storage) {
        if (!m_pictureQueue.empty()) {
            m_pictureQueue.clear();
            emit readingPicturesChanged();
        }
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    const core::ContentId source = m_pictureQueue.front();
    const std::uint64_t opening = m_opening;
    storage->loadAsset(source, [this, opening, source, language](core::Result<core::Asset> asset) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, source, language, asset = std::move(asset)] mutable {
                if (opening != m_opening || m_pictureQueue.empty()) {
                    return;
                }
                if (!asset || m_pictureReader == nullptr) {
                    std::erase(m_pictureQueue, source);
                    emit readingPicturesChanged();
                    readTheNextPicture(language);
                    return;
                }
                // Held by a pointer so that the bytes outlive the asking.
                const auto bytes =
                    std::make_shared<const std::vector<std::byte>>(std::move(asset->data));
                m_pictureReader->read(*bytes, language.toStdString(),
                                      [this, opening, source, language, bytes](
                                          core::Result<std::vector<platform::ocr::Found>> found) {
                                          hearTheNextPicture(source, language, opening,
                                                             std::move(found));
                                      });
            },
            Qt::QueuedConnection);
    });
}

void NotebookViewModel::hearTheNextPicture(const core::ContentId& source, const QString& language,
                                           std::uint64_t opening,
                                           core::Result<std::vector<platform::ocr::Found>> found) {
    if (opening != m_opening || m_pictureQueue.empty()) {
        return;
    }
    const std::vector<platform::ocr::Found> read =
        found ? *found : std::vector<platform::ocr::Found>{};
    QString words;
    for (const platform::ocr::Found& one : read) {
        if (!words.isEmpty()) {
            words += QLatin1Char{'\n'};
        }
        words += QString::fromStdString(one.text);
    }
    // A picture with nothing in it is remembered as read, so the rest of this sitting leaves it
    // alone.
    m_pictureWords[source] = words;
    if (!read.empty()) {
        keepWhatIsInPicture(source, read);
    }
    std::erase(m_pictureQueue, source);
    emit readingPicturesChanged();
    readTheNextPicture(language);
}

void NotebookViewModel::addLink(const QString& where, bool toPage, const QString& label) {
    core::Page* const page = currentPageData();
    if (page == nullptr || !canPutSomethingDown() || !m_storage) {
        return;
    }
    const QVariantMap over = areaOfWhatIsPicked();
    core::Rect area{
        .left = 0.0F,
        .top = 0.0F,
        .right = 0.0F,
        .bottom = 0.0F,
    };
    if (!over.isEmpty()) {
        area.left = static_cast<float>(over.value(QStringLiteral("columnX")).toReal());
        area.top = static_cast<float>(over.value(QStringLiteral("columnY")).toReal());
        area.right = area.left + static_cast<float>(over.value(QStringLiteral("width")).toReal());
        area.bottom = area.top + static_cast<float>(over.value(QStringLiteral("height")).toReal());
    } else if (!m_canvas.isNull()) {
        const core::Rect visible = m_canvas->visibleOnPage();
        area.left = visible.left + (visible.width() * static_cast<float>(kHalfway)) - kNewLinkWidth;
        area.top = visible.top + (visible.height() * static_cast<float>(kHalfway));
        area.right = area.left + (kNewLinkWidth * kBothSides);
        area.bottom = area.top + kNewLinkHeight;
    }

    core::Link link{
        .id = m_ids.next(),
        .at = core::Point{.x = area.left, .y = area.top},
        .width = area.width(),
        .height = area.height(),
        .kind = toPage ? core::LinkKind::Page : core::LinkKind::Web,
        .page = core::Uuid{},
        .where = toPage ? std::string{} : where.trimmed().toStdString(),
        .label = label.trimmed().toStdString(),
    };
    if (toPage) {
        for (const core::SectionInfo& section : m_outline.sections()) {
            for (const core::PageInfo& kept : section.pages) {
                if (isNamed(kept.id, where)) {
                    link.page = kept.id;
                }
            }
        }
    }
    link = core::normalized(std::move(link));
    if (!core::goesSomewhere(link)) {
        reportError(toPage ? tr("That page is not in this notebook.")
                           : tr("A link goes to a web page or to an address for mail."));
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::AddLinkCommand>(page, storage,
                                                      core::PlacedLink{
                                                          .ordinal = page->nextLinkOrdinal(),
                                                          .link = std::move(link),
                                                          .layer = layerForNewThings(*page),
                                                      }));
    publishLinks();
}

void NotebookViewModel::changeLink(const QString& linkId, const QString& where, bool toPage,
                                   const QString& label) {
    const core::Link* const standing = linkNamed(linkId);
    core::Page* const page = currentPageData();
    if (page == nullptr || standing == nullptr || !m_storage) {
        return;
    }
    core::Link wanted = *standing;
    wanted.kind = toPage ? core::LinkKind::Page : core::LinkKind::Web;
    wanted.label = label.trimmed().toStdString();
    wanted.where = toPage ? std::string{} : where.trimmed().toStdString();
    wanted.page = core::Uuid{};
    if (toPage) {
        for (const core::SectionInfo& section : m_outline.sections()) {
            for (const core::PageInfo& kept : section.pages) {
                if (isNamed(kept.id, where)) {
                    wanted.page = kept.id;
                }
            }
        }
    }
    wanted = core::normalized(std::move(wanted));
    if (!core::goesSomewhere(wanted)) {
        reportError(toPage ? tr("That page is not in this notebook.")
                           : tr("A link goes to a web page or to an address for mail."));
        return;
    }
    runCommand(std::make_unique<core::ChangeLinkCommand>(page, &*m_storage, std::move(wanted)));
    publishLinks();
}

void NotebookViewModel::removeLink(const QString& linkId) {
    const core::Link* const standing = linkNamed(linkId);
    core::Page* const page = currentPageData();
    if (page == nullptr || standing == nullptr || !m_storage) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::RemoveLinkCommand>(page, storage, standing->id));
    publishLinks();
}

QVariantMap NotebookViewModel::linkUnder(qreal columnX, qreal columnY) const {
    const core::Page* const page = currentPageData();
    if (page == nullptr) {
        return {};
    }
    const core::Link* const found = page->linkUnder(core::Point{
        .x = static_cast<float>(columnX),
        .y = static_cast<float>(columnY),
    });
    if (found == nullptr) {
        return {};
    }
    return QVariantMap{
        {QStringLiteral("linkId"), QString::fromStdString(found->id.toString())},
        {QStringLiteral("toPage"), found->kind == core::LinkKind::Page},
        {
            QStringLiteral("where"),
            found->kind == core::LinkKind::Page ? QString::fromStdString(found->page.toString())
                                                : QString::fromStdString(found->where),
        },
        {QStringLiteral("label"), QString::fromStdString(found->label)},
        {QStringLiteral("reachable"), core::goesSomewhere(*found)},
    };
}

QVariantMap NotebookViewModel::aboutLink(const QString& linkId) const {
    const core::Link* const found = linkNamed(linkId);
    if (found == nullptr) {
        return {};
    }
    return QVariantMap{
        {QStringLiteral("linkId"), QString::fromStdString(found->id.toString())},
        {QStringLiteral("toPage"), found->kind == core::LinkKind::Page},
        {
            QStringLiteral("where"),
            found->kind == core::LinkKind::Page ? QString::fromStdString(found->page.toString())
                                                : QString::fromStdString(found->where),
        },
        {QStringLiteral("label"), QString::fromStdString(found->label)},
        {QStringLiteral("reachable"), core::goesSomewhere(*found)},
    };
}

void NotebookViewModel::followLink(const QString& linkId) {
    const core::Link* const found = linkNamed(linkId);
    if (found == nullptr) {
        return;
    }
    if (!core::goesSomewhere(*found)) {
        reportError(tr("This link does not go anywhere."));
        return;
    }
    if (found->kind == core::LinkKind::Web) {
        emit goingOut(QUrl{QString::fromStdString(found->where)});
        return;
    }
    const core::Uuid wanted = found->page;
    if (m_outline.page(wanted) == nullptr) {
        reportError(tr("The page this goes to is no longer in the notebook."));
        return;
    }
    goToPage(wanted);
}

void NotebookViewModel::publishLayers() {
    std::vector<LayerItem> items;
    const core::Page* const page = currentPageData();
    if (page != nullptr) {
        const std::span<const core::Layer> standing = page->layers();
        items.reserve(standing.size());
        // Top first, the way a panel of layers is read.
        for (std::size_t step = standing.size(); step > 0; --step) {
            const core::Layer& layer = standing[step - 1];
            const auto revision = m_layerPreviews.find(layer.id);
            const QString drawn = revision == m_layerPreviews.end()
                                      ? QString{}
                                      : QStringLiteral("image://pages/layer-%1-%2")
                                            .arg(QString::fromStdString(layer.id.toString()))
                                            .arg(revision->second);
            items.push_back(itemOfLayer(layer, page->countOnLayer(layer.id), drawn));
        }
        if (layerNamed(standing, m_activeLayer).isNil() && !standing.empty()) {
            m_activeLayer = QString::fromStdString(standing.back().id.toString());
        }
    }
    m_layersModel.setItems(std::move(items));
    emit layersChanged();
}

void NotebookViewModel::publishTables() {
    std::vector<TableItem> items;
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
            std::vector<std::pair<std::size_t, const core::Table*>> standing;
            for (const core::PlacedTable& placed : found->second->tables()) {
                if (core::isShownOn(found->second->layers(), placed.layer)) {
                    standing.emplace_back(heightOnPage(*found->second, placed.layer),
                                          &placed.table);
                }
            }
            std::ranges::stable_sort(standing, {}, [](const auto& stands) { return stands.first; });
            for (const auto& [height, table] : standing) {
                items.push_back(
                    itemOfTable(*table, TablePlace{
                                            .pageId = page,
                                            .columnX = where.x() + static_cast<qreal>(table->at.x),
                                            .columnY = where.y() + static_cast<qreal>(table->at.y),
                                            .sheet = sheet,
                                        }));
            }
        }
    }
    m_tablesModel.setItems(std::move(items));
    emit pickedTableChanged();
}

void NotebookViewModel::addPicture(const QUrl& fileUrl) {
    core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
        return;
    }
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) {
        reportError(tr("Could not open %1").arg(QFileInfo{path}.fileName()));
        return;
    }
    const QByteArray data = file.readAll();
    QImage picture;
    if (data.isEmpty() || !picture.loadFromData(data)) {
        reportError(tr("%1 is not a picture").arg(QFileInfo{path}.fileName()));
        return;
    }

    core::Asset asset{
        .id = hashOf(data),
        .kind = core::AssetKind::Image,
        .name = QFileInfo{path}.fileName().toStdString(),
        .data = toBytes(data),
    };
    const core::ContentId source = asset.id;
    m_storage->submit([asset = std::move(asset)](core::NotebookStore& store) {
        return store.insertAsset(asset);
    });

    // As wide as most of the sheet, or of what is on the screen where the paper runs on, keeping
    // the shape the picture came with.
    const QRectF sheet = m_canvas->sheetRect(sheetOfPage(m_currentPage));
    const core::Rect visible = m_canvas->visibleOnPage();
    const qreal room = sheet.width() > 0.0 ? sheet.width() : static_cast<qreal>(visible.width());
    const qreal wide = std::max(room * kPictureShare, static_cast<qreal>(core::Picture::kSmallest));
    const qreal tall = wide * picture.height() / std::max(1, picture.width());
    const core::Picture placed = core::normalized(core::Picture{
        .id = m_ids.next(),
        .source = source,
        .at =
            core::Point{
                .x = static_cast<float>((room - wide) / 2.0),
                .y = visible.top + ((visible.height() - static_cast<float>(tall)) / 2.0F),
            },
        .width = static_cast<float>(wide),
        .height = static_cast<float>(tall),
        .turn = 0.0F,
    });

    m_pictureImages.insert_or_assign(source, picture);
    core::StorageThread* const storage = &*m_storage;
    noteTheMoment(placed.id);
    forgetThumbnail(page->id());
    runCommand(std::make_unique<core::AddPictureCommand>(page, storage,
                                                         core::PlacedPicture{
                                                             .ordinal = page->nextPictureOrdinal(),
                                                             .picture = placed,
                                                             .layer = layerForNewThings(),
                                                         }));
    publishPictures();
    setPickedPicture(QString::fromStdString(placed.id.toString()));
}

void NotebookViewModel::convertSelectionToText(QVariantMap style) {
    const core::Page* const page = currentPageData();
    if (page == nullptr || m_canvas.isNull() || !canPutSomethingDown() || !m_storage) {
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
        steps.push_back(std::make_unique<core::AddTextCommand>(on, &*m_storage,
                                                               core::PlacedText{
                                                                   .ordinal = on->nextTextOrdinal(),
                                                                   .box = box,
                                                                   .layer = layerForNewThings(),
                                                               }));
        forgetThumbnail(pageId);
        m_canvas->clearSelection();
        runCommand(std::make_unique<core::BundleCommand>(std::move(steps)));
        publishTexts();
        setPickedText(QString::fromStdString(box.id.toString()));
    });
}

}
