#include "platform/ink/qt/QtInkItem.hpp"

#include "platform/ink/qt/QtInkRenderer.hpp"

#include <QImage>
#include <QLineF>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::platform::ink {
namespace {

using core::InkSample;
using core::InkVertex;

constexpr std::chrono::microseconds::rep kMicrosecondsPerMillisecond = 1000;
constexpr float kZoomStep = 1.25F;
constexpr auto kLiveRedraw = std::chrono::milliseconds{12};
constexpr std::size_t kSmallestLiveStroke = 4;
constexpr float kDegreesPerWheelNotch = 120.0F;
constexpr float kWheelPixelsPerDegree = 0.5F;
constexpr float kSheetGap = 24.0F;
constexpr std::size_t kMostSheetsDrawn = 16;
// How far down the window the page that is being read is taken from.
constexpr float kReadingLine = 0.3F;

[[nodiscard]] InkSample makeSample(const QPointF& position, qreal pressure, qreal tiltX,
                                   qreal tiltY, quint64 timestampMs) {
    return InkSample{
        .x = static_cast<float>(position.x()),
        .y = static_cast<float>(position.y()),
        .pressure = static_cast<float>(pressure),
        .tiltX = static_cast<float>(tiltX),
        .tiltY = static_cast<float>(tiltY),
        .timestamp =
            std::chrono::microseconds{static_cast<std::chrono::microseconds::rep>(timestampMs)
                                      * kMicrosecondsPerMillisecond},
    };
}

[[nodiscard]] core::Point toPoint(const QPointF& point) {
    return {.x = static_cast<float>(point.x()), .y = static_cast<float>(point.y())};
}

[[nodiscard]] InkSample makeSample(const QMouseEvent& event) {
    return makeSample(event.position(), 1.0, 0.0, 0.0, event.timestamp());
}

}

QtInkItem::QtInkItem(QQuickItem* parent) : QQuickRhiItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    connect(this, &QQuickItem::windowChanged, this, &QtInkItem::observeWindow);
}

QtInkItem::~QtInkItem() {
    disconnect(this, &QQuickItem::windowChanged, this, &QtInkItem::observeWindow);
    observeWindow(nullptr);
}

QColor QtInkItem::strokeColor() const {
    const core::Color& color = m_style.color;
    return QColor::fromRgb(color.red, color.green, color.blue, color.alpha);
}

void QtInkItem::setStrokeColor(const QColor& color) {
    const core::Color converted{
        .red = static_cast<std::uint8_t>(color.red()),
        .green = static_cast<std::uint8_t>(color.green()),
        .blue = static_cast<std::uint8_t>(color.blue()),
        .alpha = static_cast<std::uint8_t>(color.alpha()),
    };
    if (converted == m_style.color) {
        return;
    }
    m_style.color = converted;
    emit strokeStyleChanged();
}

qreal QtInkItem::strokeWidth() const {
    return m_style.width;
}

void QtInkItem::setStrokeWidth(qreal width) {
    const auto converted = static_cast<float>(width);
    if (converted == m_style.width) {
        return;
    }
    m_style.width = converted;
    emit strokeStyleChanged();
}

void QtInkItem::showPage(const core::Page& page, const core::PageStyle& style,
                         std::span<const core::Uuid> hidden, std::span<const core::Stroke> extra) {
    const std::array pages{PageView{.page = &page, .style = style}};
    showColumn(pages, 0, hidden, extra);
}

// A page without a sheet of its own still takes the room of one, so that what is written on it
// does not land on the page below.
std::vector<QtInkItem::Sheet> QtInkItem::sheetsFor(std::span<const PageView> pages) {
    core::PaperSize fallback{};
    for (const PageView& view : pages) {
        if (const std::optional<core::PaperSize> paper = core::paperSize(view.style)) {
            fallback = *paper;
            break;
        }
    }

    std::vector<Sheet> sheets;
    sheets.reserve(pages.size());
    float top = 0.0F;
    for (const PageView& view : pages) {
        const std::optional<core::PaperSize> paper = core::paperSize(view.style);
        const Sheet sheet{
            .id = view.page == nullptr ? core::Uuid{} : view.page->id(),
            .style = view.style,
            .top = top,
            .width = paper ? paper->width : 0.0F,
            .height = paper ? paper->height : 0.0F,
        };
        top += (paper ? paper->height : fallback.height) + kSheetGap;
        sheets.push_back(sheet);
    }
    return sheets;
}

// A stroke keeps the mesh it was given, so erasing or undoing only rebuilds what changed.
void QtInkItem::rebuildMeshes(std::span<const PageView> pages) {
    std::vector<StrokeMesh> kept;
    for (std::size_t index = 0; index < pages.size() && index < m_sheets.size(); ++index) {
        const core::Page* const page = pages[index].page;
        if (page == nullptr) {
            continue;
        }
        const float offset = m_sheets[index].top;
        kept.reserve(kept.size() + page->strokes().size());
        for (const core::PlacedStroke& placed : page->strokes()) {
            const auto found = std::ranges::find(m_meshes, placed.stroke.id(), &StrokeMesh::id);
            if (found != m_meshes.end() && found->top == offset) {
                kept.push_back(std::move(*found));
                continue;
            }
            StrokeMesh mesh{
                .id = placed.stroke.id(),
                .vertices = {},
                .top = offset,
                .translucent = placed.stroke.style().color.alpha < core::Color::kOpaque,
            };
            core::appendStroke(mesh.vertices, placed.stroke);
            for (core::InkVertex& vertex : mesh.vertices) {
                vertex.y += offset;
            }
            kept.push_back(std::move(mesh));
        }
    }
    m_meshes = std::move(kept);
}

void QtInkItem::showColumn(std::span<const PageView> pages, int current,
                           std::span<const core::Uuid> hidden,
                           std::span<const core::Stroke> extra) {
    cancelStroke();

    std::vector<Sheet> sheets = sheetsFor(pages);
    const bool otherColumn = sheets != m_sheets;
    m_sheets = std::move(sheets);
    m_current =
        m_sheets.empty() ? 0 : std::clamp(current, 0, static_cast<int>(m_sheets.size()) - 1);
    const core::PageStyle style =
        m_sheets.empty() ? core::PageStyle{} : m_sheets[static_cast<std::size_t>(m_current)].style;
    const core::Uuid shown =
        m_sheets.empty() ? core::Uuid{} : m_sheets[static_cast<std::size_t>(m_current)].id;
    const bool otherPage = shown != m_shownPage;
    const bool otherPaper =
        style.paper != m_pageStyle.paper || style.orientation != m_pageStyle.orientation;
    m_shownPage = shown;
    m_pageStyle = style;
    if (otherPage && m_sheets.size() <= 1) {
        m_meshes.clear();
    }
    if ((otherPage || otherPaper || !m_viewFitted) && (otherColumn || m_sheets.size() <= 1)) {
        fitPage();
    } else if (otherPage && !m_followingScroll) {
        goToSheet(m_current);
    }

    rebuildMeshes(pages);

    m_hidden.assign(hidden.begin(), hidden.end());
    m_extra.assign(extra.begin(), extra.end());
    if (otherPage) {
        m_selected.clear();
        m_marquee.reset();
        m_dragFrom.reset();
        m_dragOffset = {};
        emit selectionChanged();
    } else {
        std::erase_if(m_selected, [this](const core::Uuid& id) {
            return std::ranges::none_of(m_meshes,
                                        [&id](const StrokeMesh& mesh) { return mesh.id == id; });
        });
    }
    rebuildMedia();
    rebuildBuffers();
}

float QtInkItem::currentTop() const noexcept {
    return m_sheets.empty() ? 0.0F : m_sheets[static_cast<std::size_t>(m_current)].top;
}

int QtInkItem::sheetAt(float y) const noexcept {
    for (std::size_t index = 0; index < m_sheets.size(); ++index) {
        const Sheet& sheet = m_sheets[index];
        const float bottom = sheet.top + sheet.height + (kSheetGap / 2.0F);
        if (y < bottom) {
            return static_cast<int>(index);
        }
    }
    return m_sheets.empty() ? -1 : static_cast<int>(m_sheets.size()) - 1;
}

std::optional<core::PaperSize> QtInkItem::columnSize() const noexcept {
    if (m_sheets.size() <= 1) {
        return core::paperSize(m_pageStyle);
    }
    float width = 0.0F;
    float height = 0.0F;
    for (const Sheet& sheet : m_sheets) {
        width = std::max(width, sheet.width);
        height = std::max(height, sheet.top + sheet.height);
    }
    if (width <= 0.0F || height <= 0.0F) {
        return std::nullopt;
    }
    return core::PaperSize{.width = width, .height = height};
}

InkSample QtInkItem::onSheet(InkSample sample) const noexcept {
    sample.y -= currentTop();
    return sample;
}

core::Point QtInkItem::onSheet(core::Point point) const noexcept {
    point.y -= currentTop();
    return point;
}

core::Stroke QtInkItem::onSheet(const core::Stroke& stroke) const {
    const float top = currentTop();
    if (top == 0.0F) {
        return stroke;
    }
    core::Stroke moved{stroke.id(), stroke.style()};
    for (InkSample sample : stroke.samples()) {
        sample.y -= top;
        moved.append(sample);
    }
    return moved;
}

core::Rect QtInkItem::visibleOnPage() const noexcept {
    const core::Rect seen = visiblePage();
    const float top = currentTop();
    return core::Rect{
        .left = seen.left,
        .top = seen.top - top,
        .right = seen.right,
        .bottom = seen.bottom - top,
    };
}

std::vector<QtInkItem::VisibleSheet> QtInkItem::visibleSheets() const {
    std::vector<VisibleSheet> shown;
    if (m_sheets.empty()) {
        return shown;
    }
    const core::Rect view = visiblePage().inflated(kSheetGap);
    for (const Sheet& sheet : m_sheets) {
        if (sheet.width <= 0.0F || sheet.height <= 0.0F || shown.size() >= kMostSheetsDrawn) {
            continue;
        }
        const core::Rect area{
            .left = 0.0F,
            .top = sheet.top,
            .right = sheet.width,
            .bottom = sheet.top + sheet.height,
        };
        if (area.intersects(view)) {
            shown.push_back(VisibleSheet{.area = area, .style = sheet.style});
        }
    }
    return shown;
}

void QtInkItem::goToSheet(int index) {
    if (m_sheets.empty()) {
        return;
    }
    const auto at =
        static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(m_sheets.size()) - 1));
    core::Viewport viewport = m_viewport;
    viewport.showTop(m_sheets[at].top - (core::Viewport::kPaperMargin / viewport.scale()));
    changeView(viewport);
}

namespace {

constexpr core::Color kMarqueeColor{.red = 60, .green = 110, .blue = 220, .alpha = 220};
constexpr core::Color kMarqueeFill{.red = 60, .green = 110, .blue = 220, .alpha = 30};
constexpr core::Color kSelectionColor{.red = 60, .green = 110, .blue = 220, .alpha = 200};
constexpr core::Color kSelectionFill{.red = 60, .green = 110, .blue = 220, .alpha = 22};
constexpr float kOutlinePixels = 1.5F;
constexpr float kSelectionMargin = 6.0F;

void appendLine(std::vector<InkVertex>& into, core::Point from, core::Point to, float width,
                const core::Color& color) {
    const core::StrokeStyle style{.color = color, .width = width};
    core::appendSegment(into, InkSample{.x = from.x, .y = from.y}, InkSample{.x = to.x, .y = to.y},
                        style);
}

void appendFill(std::vector<InkVertex>& into, const core::Rect& box, const core::Color& color) {
    const core::StrokeStyle style{.color = color, .width = box.height()};
    const float middle = (box.top + box.bottom) / 2.0F;
    core::appendSegment(into, InkSample{.x = box.left, .y = middle},
                        InkSample{.x = box.right, .y = middle}, style);
}

void appendOutline(std::vector<InkVertex>& into, const core::Rect& box, float width,
                   const core::Color& color) {
    const core::Point topLeft{.x = box.left, .y = box.top};
    const core::Point topRight{.x = box.right, .y = box.top};
    const core::Point bottomRight{.x = box.right, .y = box.bottom};
    const core::Point bottomLeft{.x = box.left, .y = box.bottom};
    appendLine(into, topLeft, topRight, width, color);
    appendLine(into, topRight, bottomRight, width, color);
    appendLine(into, bottomRight, bottomLeft, width, color);
    appendLine(into, bottomLeft, topLeft, width, color);
}

}

void QtInkItem::rebuildBuffers() {
    m_vertices.clear();
    m_highlights.clear();
    m_overlay.clear();

    const bool dragging = m_dragFrom.has_value();
    for (const StrokeMesh& mesh : m_meshes) {
        if (std::ranges::find(m_hidden, mesh.id) != m_hidden.end()) {
            continue;
        }
        const bool picked = std::ranges::find(m_selected, mesh.id) != m_selected.end();
        if (dragging && picked) {
            for (InkVertex vertex : mesh.vertices) {
                vertex.x += m_dragOffset.x;
                vertex.y += m_dragOffset.y;
                m_overlay.push_back(vertex);
            }
            continue;
        }
        std::vector<InkVertex>& into = mesh.translucent ? m_highlights : m_vertices;
        into.insert(into.end(), mesh.vertices.begin(), mesh.vertices.end());
    }

    const float top = currentTop();
    for (const core::Stroke& stroke : m_extra) {
        std::vector<InkVertex>& into =
            stroke.style().color.alpha < core::Color::kOpaque ? m_highlights : m_vertices;
        const std::size_t first = into.size();
        core::appendStroke(into, stroke);
        for (std::size_t i = first; i < into.size(); ++i) {
            into[i].y += top;
        }
    }

    const float outline = kOutlinePixels / std::max(m_viewport.scale(), 0.01F);

    if (!m_selected.empty()) {
        if (const std::optional<core::Rect> bounds = selectionBounds()) {
            const core::Rect shown =
                core::Rect{
                    .left = bounds->left + m_dragOffset.x,
                    .top = bounds->top + m_dragOffset.y,
                    .right = bounds->right + m_dragOffset.x,
                    .bottom = bounds->bottom + m_dragOffset.y,
                }
                    .inflated(kSelectionMargin);
            appendFill(m_overlay, shown, kSelectionFill);
            appendOutline(m_overlay, shown, outline, kSelectionColor);
        }
    }

    if (m_marquee) {
        const core::Rect box = core::shapedBox(m_marquee->from, m_marquee->to, m_shapeKeys);
        appendFill(m_overlay, box, kMarqueeFill);
        appendOutline(m_overlay, box, outline, kMarqueeColor);
    }

    ++m_generation;
    update();
}

void QtInkItem::showSelection(std::vector<core::Uuid> strokeIds) {
    m_selected = std::move(strokeIds);
    m_marquee.reset();
    m_dragFrom.reset();
    m_dragOffset = {};
    emit selectionChanged();
    rebuildBuffers();
}

void QtInkItem::forgetStrokes(std::span<const core::Uuid> strokeIds) {
    std::erase_if(m_meshes, [&strokeIds](const StrokeMesh& mesh) {
        return std::ranges::find(strokeIds, mesh.id) != strokeIds.end();
    });
}

void QtInkItem::setSmoothing(qreal smoothing) {
    const qreal wanted = std::clamp(smoothing, 0.0, 1.0);
    if (qFuzzyCompare(wanted + 1.0, m_smoothing + 1.0)) {
        return;
    }
    m_smoothing = wanted;
    m_filter.setParameters(core::smoothingOf(static_cast<float>(m_smoothing)));
    emit smoothingChanged();
}

void QtInkItem::setShape(int shape) {
    const auto wanted =
        static_cast<core::Shape>(std::clamp(shape, 0, static_cast<int>(core::Shape::Ellipse)));
    if (wanted == m_shape) {
        return;
    }
    m_shape = wanted;
    emit shapeChanged();
}

void QtInkItem::setDeskColor(const QColor& colour) {
    if (colour == m_deskColor) {
        return;
    }
    m_deskColor = colour;
    emit deskColorChanged();
    update();
}

void QtInkItem::setPicking(bool picking) {
    if (picking == m_picking) {
        return;
    }
    m_picking = picking;
    emit pickingChanged();
}

void QtInkItem::setPanning(bool panning) {
    if (panning == m_panning) {
        return;
    }
    m_panning = panning;
    m_panFrom.reset();
    emit panningChanged();
}

void QtInkItem::setSelecting(bool selecting) {
    if (selecting == m_selecting) {
        return;
    }
    m_selecting = selecting;
    if (!m_selecting) {
        clearSelection();
    }
    emit selectingChanged();
}

void QtInkItem::clearSelection() {
    if (m_selected.empty() && !m_marquee) {
        return;
    }
    m_selected.clear();
    m_marquee.reset();
    m_dragFrom.reset();
    m_dragOffset = {};
    emit selectionChanged();
    rebuildBuffers();
}

std::optional<core::Rect> QtInkItem::selectionBounds() const noexcept {
    std::optional<core::Rect> bounds;
    for (const StrokeMesh& mesh : m_meshes) {
        if (std::ranges::find(m_selected, mesh.id) == m_selected.end()) {
            continue;
        }
        for (const InkVertex& vertex : mesh.vertices) {
            const core::Rect point{
                .left = vertex.x,
                .top = vertex.y,
                .right = vertex.x,
                .bottom = vertex.y,
            };
            bounds = bounds ? bounds->united(point) : point;
        }
    }
    return bounds;
}

QRectF QtInkItem::selectionRect() const {
    const std::optional<core::Rect> bounds = selectionBounds();
    if (!bounds) {
        return {};
    }
    const core::Rect box = bounds->inflated(kSelectionMargin);
    const core::Point topLeft = m_viewport.toView(core::Point{.x = box.left, .y = box.top});
    const core::Point bottomRight = m_viewport.toView(core::Point{.x = box.right, .y = box.bottom});
    return QRectF{QPointF{topLeft.x, topLeft.y}, QPointF{bottomRight.x, bottomRight.y}};
}

bool QtInkItem::overSelection(const InkSample& sample) const noexcept {
    const std::optional<core::Rect> bounds = selectionBounds();
    if (!bounds) {
        return false;
    }
    const core::Rect box = bounds->inflated(kSelectionMargin);
    return sample.x >= box.left && sample.x <= box.right && sample.y >= box.top
           && sample.y <= box.bottom;
}

void QtInkItem::beginMarquee(const InkSample& sample) {
    m_selected.clear();
    const core::Point corner{.x = sample.x, .y = sample.y};
    m_marquee = Marquee{.from = corner, .to = corner};
    emit selectionChanged();
    rebuildBuffers();
}

void QtInkItem::growMarquee(const InkSample& sample) {
    if (!m_marquee) {
        return;
    }
    m_marquee->to = core::Point{.x = sample.x, .y = sample.y};
    rebuildBuffers();
}

void QtInkItem::finishMarquee() {
    if (!m_marquee) {
        return;
    }
    const core::Rect box = core::shapedBox(m_marquee->from, m_marquee->to, m_shapeKeys);
    m_marquee.reset();
    if (m_sink != nullptr && box.width() > 0.0F && box.height() > 0.0F) {
        const std::array corners{
            core::Point{.x = box.left, .y = box.top},
            core::Point{.x = box.right, .y = box.top},
            core::Point{.x = box.right, .y = box.bottom},
            core::Point{.x = box.left, .y = box.bottom},
        };
        const std::array local{
            onSheet(corners[0]),
            onSheet(corners[1]),
            onSheet(corners[2]),
            onSheet(corners[3]),
        };
        m_sink->selectionDrawn(local);
    }
    rebuildBuffers();
}

void QtInkItem::beginDrag(const InkSample& sample) {
    m_dragFrom = core::Point{.x = sample.x, .y = sample.y};
    m_dragOffset = {};
    rebuildBuffers();
}

void QtInkItem::dragTo(const InkSample& sample) {
    if (!m_dragFrom) {
        return;
    }
    m_dragOffset = core::Point{.x = sample.x - m_dragFrom->x, .y = sample.y - m_dragFrom->y};
    rebuildBuffers();
}

void QtInkItem::finishDrag() {
    if (!m_dragFrom) {
        return;
    }
    const core::Point moved = m_dragOffset;
    m_dragFrom.reset();
    m_dragOffset = {};
    if (m_sink != nullptr && (moved.x != 0.0F || moved.y != 0.0F)) {
        m_sink->selectionMoved(moved.x, moved.y);
    }
    rebuildBuffers();
}

void QtInkItem::showView(const core::Viewport& viewport) {
    m_viewFitted = true;
    changeView(viewport);
}

void QtInkItem::showMedia(std::span<const MediaPiece> pieces) {
    m_media.assign(pieces.begin(), pieces.end());
    rebuildMedia();
    ++m_mediaGeneration;
    emit mediaChanged();
    update();
}

void QtInkItem::clearMedia() {
    if (m_media.empty()) {
        return;
    }
    m_media.clear();
    rebuildMedia();
    ++m_mediaGeneration;
    emit mediaChanged();
    update();
}

// Every picture is drawn where its own page stands in the column.
void QtInkItem::rebuildMedia() {
    m_mediaDraws.clear();
    m_mediaDraws.reserve(m_media.size());
    for (const MediaPiece& piece : m_media) {
        const auto sheet = std::ranges::find(m_sheets, piece.page, &Sheet::id);
        if (sheet == m_sheets.end() || piece.picture.isNull() || piece.area.isEmpty()) {
            continue;
        }
        m_mediaDraws.push_back(MediaDraw{
            .picture = piece.picture,
            .area = piece.area.translated(0.0, static_cast<double>(sheet->top)),
        });
    }
}

QImage QtInkItem::media() const {
    const auto found = std::ranges::find(m_media, m_shownPage, &MediaPiece::page);
    return found == m_media.end() ? QImage{} : found->picture;
}

QRectF QtInkItem::mediaArea() const {
    const auto found = std::ranges::find(m_media, m_shownPage, &MediaPiece::page);
    return found == m_media.end() ? QRectF{} : found->area;
}

void QtInkItem::clear() {
    cancelStroke();
    m_meshes.clear();
    m_vertices.clear();
    m_highlights.clear();
    m_overlay.clear();
    m_selected.clear();
    m_marquee.reset();
    ++m_generation;
    update();
}

std::string_view QtInkItem::name() const noexcept {
    return "qt";
}

void QtInkItem::setSink(IInkSink* sink) noexcept {
    m_sink = sink;
}

void QtInkItem::setStrokeStyle(const core::StrokeStyle& style) {
    if (style == m_style) {
        return;
    }
    m_style = style;
    emit strokeStyleChanged();
}

void QtInkItem::setErasing(bool erasing) {
    if (erasing == m_erasing) {
        return;
    }
    m_erasing = erasing;
    emit erasingChanged();
}

void QtInkItem::setEraserRadius(qreal radius) {
    if (qFuzzyCompare(radius, m_eraserRadius)) {
        return;
    }
    m_eraserRadius = radius;
    emit eraserRadiusChanged();
}

void QtInkItem::setPressureSensitive(bool sensitive) {
    if (sensitive == m_pressureSensitive) {
        return;
    }
    m_pressureSensitive = sensitive;
    emit pressureSensitiveChanged();
}

QPointF QtInkItem::viewOrigin() const noexcept {
    return {m_viewport.origin().x, m_viewport.origin().y};
}

core::Rect QtInkItem::visiblePage() const noexcept {
    return m_viewport.visiblePage(viewSize());
}

core::ViewSize QtInkItem::viewSize() const noexcept {
    return {.width = static_cast<float>(width()), .height = static_cast<float>(height())};
}

void QtInkItem::changeView(const core::Viewport& viewport) {
    core::Viewport kept = viewport;
    kept.keepPaperInView(viewSize(), columnSize());
    if (kept == m_viewport) {
        return;
    }
    m_viewport = kept;
    emit viewChanged();
    update();
    followScrolling();
}

// In a column, the page that is being read is the one a third of the way down the window.
void QtInkItem::followScrolling() {
    if (m_sheets.size() <= 1 || m_followingScroll) {
        return;
    }
    const core::Rect view = visiblePage();
    const int under = sheetAt(view.top + (view.height() * kReadingLine));
    if (under < 0 || under == m_current) {
        return;
    }
    m_followingScroll = true;
    emit pageWanted(under);
    m_followingScroll = false;
}

void QtInkItem::zoomIn() {
    core::Viewport viewport = m_viewport;
    viewport.zoomAround(toPoint(boundingRect().center()), kZoomStep);
    changeView(viewport);
}

void QtInkItem::zoomOut() {
    core::Viewport viewport = m_viewport;
    viewport.zoomAround(toPoint(boundingRect().center()), 1.0F / kZoomStep);
    changeView(viewport);
}

void QtInkItem::fitPage() {
    if (width() <= 0.0 || height() <= 0.0) {
        m_viewFitted = false;
        return;
    }
    core::Viewport viewport;
    viewport.fit(viewSize(), core::paperSize(m_pageStyle));
    viewport.showTop(currentTop() - (core::Viewport::kPaperMargin / viewport.scale()));
    viewport.keepPaperInView(viewSize(), columnSize());
    m_viewFitted = true;
    m_viewport = viewport;
    emit viewChanged();
    update();
}

void QtInkItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickRhiItem::geometryChange(newGeometry, oldGeometry);
    if (!m_viewFitted) {
        fitPage();
        return;
    }
    changeView(m_viewport);
}

InkSample QtInkItem::onPage(InkSample sample) const noexcept {
    const core::Point page = m_viewport.toPage({.x = sample.x, .y = sample.y});
    sample.x = page.x;
    sample.y = page.y;
    if (!m_pressureSensitive) {
        sample.pressure = 1.0F;
    }
    return sample;
}

void QtInkItem::wheelEvent(QWheelEvent* event) {
    const QPoint angle = event->angleDelta();
    core::Viewport viewport = m_viewport;
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const float notches = static_cast<float>(angle.y()) / kDegreesPerWheelNotch;
        const QPointF position = event->position();
        viewport.zoomAround(toPoint(position), std::pow(kZoomStep, notches));
    } else {
        QPointF delta = event->pixelDelta().isNull() ? QPointF{angle} * kWheelPixelsPerDegree
                                                     : QPointF{event->pixelDelta()};
        if (event->modifiers().testFlag(Qt::ShiftModifier) && delta.x() == 0.0) {
            delta = {delta.y(), 0.0};
        }
        viewport.panBy(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    }
    changeView(viewport);
    event->accept();
}

bool QtInkItem::handleNativeGesture(const QNativeGestureEvent& event) {
    const core::Point anchor = toPoint(event.position());
    core::Viewport viewport = m_viewport;
    switch (event.gestureType()) {
    case Qt::ZoomNativeGesture:
        viewport.zoomAround(anchor, 1.0F + static_cast<float>(event.value()));
        changeView(viewport);
        return true;
    case Qt::SmartZoomNativeGesture:
        fitPage();
        return true;
    case Qt::PanNativeGesture:
        viewport.panBy(static_cast<float>(event.delta().x()),
                       static_cast<float>(event.delta().y()));
        changeView(viewport);
        return true;
    default:
        return false;
    }
}

bool QtInkItem::event(QEvent* event) {
    auto* const gesture = dynamic_cast<QNativeGestureEvent*>(event);
    if (gesture != nullptr && handleNativeGesture(*gesture)) {
        event->accept();
        return true;
    }
    return QQuickRhiItem::event(event);
}

void QtInkItem::touchEvent(QTouchEvent* event) {
    const QList<QEventPoint>& points = event->points();
    if (points.isEmpty() || event->type() == QEvent::TouchEnd
        || event->type() == QEvent::TouchCancel) {
        m_touchCentroid.reset();
        event->accept();
        return;
    }

    QPointF centroid;
    for (const QEventPoint& point : points) {
        centroid += point.position();
    }
    centroid /= static_cast<qreal>(points.size());
    qreal spread = 0.0;
    for (const QEventPoint& point : points) {
        spread += QLineF{centroid, point.position()}.length();
    }
    spread /= static_cast<qreal>(points.size());

    const bool fingersChanged = std::ranges::any_of(points, [](const QEventPoint& point) {
        return point.state() == QEventPoint::Pressed || point.state() == QEventPoint::Released;
    });
    if (m_touchCentroid && !fingersChanged) {
        core::Viewport viewport = m_viewport;
        const QPointF delta = centroid - *m_touchCentroid;
        viewport.panBy(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
        if (points.size() > 1 && m_touchSpread > 0.0 && spread > 0.0) {
            viewport.zoomAround(toPoint(centroid), static_cast<float>(spread / m_touchSpread));
        }
        changeView(viewport);
    }
    m_touchCentroid = centroid;
    m_touchSpread = spread;
    event->accept();
}

void QtInkItem::touchUngrabEvent() {
    m_touchCentroid.reset();
}

QQuickRhiItemRenderer* QtInkItem::createRenderer() {
    return new QtInkRenderer;
}

void QtInkItem::mousePressEvent(QMouseEvent* event) {
    noteKeys(event->modifiers());
    press(onPage(makeSample(*event)), false);
    event->accept();
}

void QtInkItem::mouseMoveEvent(QMouseEvent* event) {
    noteKeys(event->modifiers());
    move(onPage(makeSample(*event)));
    event->accept();
}

void QtInkItem::mouseReleaseEvent(QMouseEvent* event) {
    noteKeys(event->modifiers());
    release(onPage(makeSample(*event)));
    event->accept();
}

void QtInkItem::noteKeys(Qt::KeyboardModifiers modifiers) {
    const core::ShapeKeys keys{
        .even = modifiers.testFlag(Qt::ShiftModifier),
        .fromCentre = modifiers.testFlag(Qt::AltModifier),
    };
    if (keys.even == m_shapeKeys.even && keys.fromCentre == m_shapeKeys.fromCentre) {
        return;
    }
    m_shapeKeys = keys;
    if (m_activeStroke && m_shape != core::Shape::Freehand) {
        redrawActiveStroke();
        update();
    }
}

void QtInkItem::mouseUngrabEvent() {
    cancelStroke();
    finishErase();
}

bool QtInkItem::eventFilter(QObject* watched, QEvent* event) {
    auto* const tabletEvent = dynamic_cast<QTabletEvent*>(event);
    if (tabletEvent != nullptr && watched == m_observedWindow && handleTabletEvent(*tabletEvent)) {
        return true;
    }
    return QQuickRhiItem::eventFilter(watched, event);
}

void QtInkItem::observeWindow(QQuickWindow* window) {
    if (m_observedWindow) {
        m_observedWindow->removeEventFilter(this);
    }
    m_observedWindow = window;
    if (m_observedWindow) {
        m_observedWindow->installEventFilter(this);
    }
}

bool QtInkItem::handleTabletEvent(QTabletEvent& event) {
    const QPointF position = mapFromScene(event.scenePosition());
    const InkSample sample = onPage(
        makeSample(position, event.pressure(), event.xTilt(), event.yTilt(), event.timestamp()));
    noteKeys(event.modifiers());

    switch (event.type()) {
    case QEvent::TabletPress:
        if (!isVisible() || !isEnabled() || !contains(position)) {
            return false;
        }
        press(sample, event.pointerType() == QPointingDevice::PointerType::Eraser);
        break;
    case QEvent::TabletMove:
        if (!isTracking()) {
            return false;
        }
        move(sample);
        break;
    case QEvent::TabletRelease:
        if (!isTracking()) {
            return false;
        }
        release(sample);
        break;
    default:
        return false;
    }

    event.accept();
    return true;
}

void QtInkItem::press(const InkSample& sample, bool eraserTip) {
    // A press on another sheet of the column reads that page first, and works on it from there.
    if (const int under = sheetAt(sample.y); under >= 0 && under != m_current) {
        emit pageWanted(under);
        if (under != m_current) {
            return;
        }
    }
    if (m_picking && !eraserTip) {
        if (m_sink != nullptr) {
            m_sink->colourWanted(onSheet(sample));
        }
        return;
    }
    if (m_panning && !eraserTip) {
        m_panFrom = core::Point{.x = sample.x, .y = sample.y};
        return;
    }
    if (m_selecting && !eraserTip) {
        if (overSelection(sample)) {
            beginDrag(sample);
        } else {
            beginMarquee(sample);
        }
        return;
    }
    if (m_erasing || eraserTip) {
        beginErase(sample);
    } else {
        beginStroke(sample);
    }
}

void QtInkItem::move(const InkSample& sample) {
    if (m_panFrom) {
        // The point the drag started on has to stay under the pointer, so the view moves the
        // other way.
        core::Viewport viewport = m_viewport;
        viewport.panBy((sample.x - m_panFrom->x) * m_viewport.scale(),
                       (sample.y - m_panFrom->y) * m_viewport.scale());
        changeView(viewport);
        return;
    }
    if (m_dragFrom) {
        dragTo(sample);
    } else if (m_marquee) {
        growMarquee(sample);
    } else if (m_eraserPosition) {
        moveEraser(sample);
    } else {
        appendToStroke(sample);
    }
}

void QtInkItem::release(const InkSample& sample) {
    if (m_panFrom) {
        move(sample);
        m_panFrom.reset();
        return;
    }
    if (m_dragFrom) {
        dragTo(sample);
        finishDrag();
    } else if (m_marquee) {
        growMarquee(sample);
        finishMarquee();
    } else if (m_eraserPosition) {
        moveEraser(sample);
        finishErase();
    } else {
        endStroke(sample);
    }
}

bool QtInkItem::isTracking() const noexcept {
    return m_activeStroke.has_value() || m_eraserPosition.has_value() || m_dragFrom.has_value()
           || m_panFrom.has_value() || m_marquee.has_value();
}

void QtInkItem::beginErase(const InkSample& sample) {
    cancelStroke();
    finishErase();
    m_eraserPosition = sample;
    if (m_sink != nullptr) {
        const InkSample local = onSheet(sample);
        m_sink->eraserMoved(local, local, static_cast<float>(m_eraserRadius));
    }
}

void QtInkItem::moveEraser(const InkSample& sample) {
    if (!m_eraserPosition) {
        return;
    }
    const InkSample from = std::exchange(*m_eraserPosition, sample);
    if (m_sink != nullptr) {
        m_sink->eraserMoved(onSheet(from), onSheet(sample), static_cast<float>(m_eraserRadius));
    }
}

void QtInkItem::finishErase() {
    if (!m_eraserPosition) {
        return;
    }
    m_eraserPosition.reset();
    if (m_sink != nullptr) {
        m_sink->eraseFinished();
    }
}

std::vector<InkVertex>& QtInkItem::activeVertices() noexcept {
    return m_activeIsTranslucent ? m_highlights : m_vertices;
}

void QtInkItem::beginStroke(const InkSample& sample) {
    finishErase();
    cancelStroke();
    m_filter.reset();
    m_activeStroke.emplace(m_ids.next(), m_style);
    m_activeStroke->append(m_filter.filter(sample));
    m_activeIsTranslucent = m_style.color.alpha < core::Color::kOpaque;
    m_activeStrokeFirstVertex = activeVertices().size();
    m_liveSamples = 1;
    m_liveDrawnAt = std::chrono::steady_clock::now();
    if (m_sink != nullptr) {
        m_sink->strokeStarted(onSheet(sample));
    }
}

void QtInkItem::appendToStroke(const InkSample& sample) {
    if (!m_activeStroke) {
        return;
    }
    m_activeStroke->append(m_filter.filter(sample));
    if (m_shape == core::Shape::Freehand) {
        // The body of the stroke is redrawn along its curve every so often, while the newest
        // samples are strung on as they arrive so the tip keeps up with the pen.
        const auto now = std::chrono::steady_clock::now();
        const bool due = now - m_liveDrawnAt >= kLiveRedraw
                         || m_activeStroke->samples().size() <= kSmallestLiveStroke;
        refreshLiveStroke(due);
        if (due) {
            m_liveDrawnAt = now;
        }
    } else {
        redrawActiveStroke();
    }
    if (m_sink != nullptr) {
        m_sink->sampleAdded(onSheet(sample));
    }
    update();
}

void QtInkItem::refreshLiveStroke(bool full) {
    if (!m_activeStroke) {
        return;
    }
    const core::Stroke& stroke = *m_activeStroke;
    const core::StrokeStyle style = stroke.style();
    const std::span<const InkSample> samples = stroke.samples();
    std::vector<InkVertex>& into = activeVertices();

    if (full) {
        into.resize(m_activeStrokeFirstVertex);
        core::appendStroke(into, stroke);
        m_liveSamples = samples.size();
        ++m_generation;
        return;
    }

    for (std::size_t i = std::max<std::size_t>(m_liveSamples, 1); i < samples.size(); ++i) {
        core::appendSegment(into, samples[i - 1], samples[i], style);
    }
    m_liveSamples = samples.size();
    ++m_generation;
}

void QtInkItem::redrawActiveStroke() {
    if (!m_activeStroke) {
        return;
    }
    const core::Stroke preview = core::shaped(*m_activeStroke, m_shape, m_shapeKeys);
    std::vector<InkVertex>& into = activeVertices();
    into.resize(m_activeStrokeFirstVertex);
    core::appendStroke(into, preview);
    ++m_generation;
}

void QtInkItem::endStroke(const InkSample& sample) {
    if (!m_activeStroke) {
        return;
    }
    m_activeStroke->append(m_filter.filter(sample));
    const core::Stroke drawn = std::move(*m_activeStroke);
    m_activeStroke.reset();
    const core::Stroke finished = core::shaped(drawn, m_shape, m_shapeKeys);

    std::vector<InkVertex>& into = activeVertices();
    into.resize(m_activeStrokeFirstVertex);
    StrokeMesh mesh{
        .id = finished.id(),
        .vertices = {},
        .top = currentTop(),
        .translucent = m_activeIsTranslucent,
    };
    core::appendStroke(mesh.vertices, finished);
    into.insert(into.end(), mesh.vertices.begin(), mesh.vertices.end());
    m_meshes.push_back(std::move(mesh));
    ++m_generation;
    if (m_sink != nullptr) {
        m_sink->strokeFinished(onSheet(sample));
        m_sink->strokeCompleted(onSheet(finished));
    }
    update();
}

void QtInkItem::cancelStroke() {
    if (!m_activeStroke) {
        return;
    }
    m_activeStroke.reset();
    activeVertices().resize(m_activeStrokeFirstVertex);
    ++m_generation;
    if (m_sink != nullptr) {
        m_sink->strokeCancelled();
    }
    update();
}

}
