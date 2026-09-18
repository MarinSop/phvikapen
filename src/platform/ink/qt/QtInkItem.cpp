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
constexpr float kDegreesPerWheelNotch = 120.0F;
constexpr float kWheelPixelsPerDegree = 0.5F;

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
                         std::span<const core::Uuid> hidden) {
    cancelStroke();

    const bool otherPage = page.id() != m_shownPage;
    const bool otherPaper =
        style.paper != m_pageStyle.paper || style.orientation != m_pageStyle.orientation;
    m_shownPage = page.id();
    m_pageStyle = style;
    if (otherPage) {
        m_meshes.clear();
    }
    if (otherPage || otherPaper || !m_viewFitted) {
        fitPage();
    }

    // A stroke keeps the mesh it was given, so erasing or undoing only rebuilds what changed.
    std::vector<StrokeMesh> kept;
    kept.reserve(page.strokes().size());
    for (const core::PlacedStroke& placed : page.strokes()) {
        const auto found = std::ranges::find(m_meshes, placed.stroke.id(), &StrokeMesh::id);
        if (found != m_meshes.end()) {
            kept.push_back(std::move(*found));
            continue;
        }
        StrokeMesh mesh{
            .id = placed.stroke.id(),
            .vertices = {},
            .translucent = placed.stroke.style().color.alpha < core::Color::kOpaque,
        };
        core::appendStroke(mesh.vertices, placed.stroke);
        kept.push_back(std::move(mesh));
    }
    m_meshes = std::move(kept);

    m_vertices.clear();
    m_highlights.clear();
    for (const StrokeMesh& mesh : m_meshes) {
        if (std::ranges::find(hidden, mesh.id) != hidden.end()) {
            continue;
        }
        std::vector<InkVertex>& into = mesh.translucent ? m_highlights : m_vertices;
        into.insert(into.end(), mesh.vertices.begin(), mesh.vertices.end());
    }

    ++m_generation;
    update();
}

void QtInkItem::showView(const core::Viewport& viewport) {
    m_viewFitted = true;
    changeView(viewport);
}

void QtInkItem::showMedia(const QImage& image) {
    m_media = image;
    ++m_mediaGeneration;
    update();
}

void QtInkItem::clearMedia() {
    if (m_media.isNull()) {
        return;
    }
    m_media = QImage{};
    ++m_mediaGeneration;
    update();
}

void QtInkItem::clear() {
    cancelStroke();
    m_meshes.clear();
    m_vertices.clear();
    m_highlights.clear();
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

core::ViewSize QtInkItem::viewSize() const noexcept {
    return {.width = static_cast<float>(width()), .height = static_cast<float>(height())};
}

void QtInkItem::changeView(const core::Viewport& viewport) {
    core::Viewport kept = viewport;
    kept.keepPaperInView(viewSize(), core::paperSize(m_pageStyle));
    if (kept == m_viewport) {
        return;
    }
    m_viewport = kept;
    emit viewChanged();
    update();
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
    press(onPage(makeSample(*event)), false);
    event->accept();
}

void QtInkItem::mouseMoveEvent(QMouseEvent* event) {
    move(onPage(makeSample(*event)));
    event->accept();
}

void QtInkItem::mouseReleaseEvent(QMouseEvent* event) {
    release(onPage(makeSample(*event)));
    event->accept();
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

bool QtInkItem::onPaper(const InkSample& sample) const noexcept {
    const std::optional<core::PaperSize> paper = core::paperSize(m_pageStyle);
    if (!paper) {
        return true;
    }
    return sample.x >= 0.0F && sample.y >= 0.0F && sample.x <= paper->width
           && sample.y <= paper->height;
}

void QtInkItem::press(const InkSample& sample, bool eraserTip) {
    if (!m_erasing && !eraserTip && !onPaper(sample)) {
        return;
    }
    if (m_erasing || eraserTip) {
        beginErase(sample);
    } else {
        beginStroke(sample);
    }
}

void QtInkItem::move(const InkSample& sample) {
    if (m_eraserPosition) {
        moveEraser(sample);
    } else {
        appendToStroke(sample);
    }
}

void QtInkItem::release(const InkSample& sample) {
    if (m_eraserPosition) {
        moveEraser(sample);
        finishErase();
    } else {
        endStroke(sample);
    }
}

bool QtInkItem::isTracking() const noexcept {
    return m_activeStroke.has_value() || m_eraserPosition.has_value();
}

void QtInkItem::beginErase(const InkSample& sample) {
    cancelStroke();
    finishErase();
    m_eraserPosition = sample;
    if (m_sink != nullptr) {
        m_sink->eraserMoved(sample, sample, static_cast<float>(m_eraserRadius));
    }
}

void QtInkItem::moveEraser(const InkSample& sample) {
    if (!m_eraserPosition) {
        return;
    }
    const InkSample from = std::exchange(*m_eraserPosition, sample);
    if (m_sink != nullptr) {
        m_sink->eraserMoved(from, sample, static_cast<float>(m_eraserRadius));
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
    if (m_sink != nullptr) {
        m_sink->strokeStarted(sample);
    }
}

void QtInkItem::appendToStroke(const InkSample& sample) {
    if (!m_activeStroke) {
        return;
    }
    const InkSample previous = m_activeStroke->samples().back();
    const InkSample smoothed = m_filter.filter(sample);
    m_activeStroke->append(smoothed);
    const core::StrokeStyle style = m_activeStroke->style();
    core::appendSegment(activeVertices(), previous, smoothed, style);
    if (m_sink != nullptr) {
        m_sink->sampleAdded(sample);
    }
    update();
}

void QtInkItem::endStroke(const InkSample& sample) {
    if (!m_activeStroke) {
        return;
    }
    m_activeStroke->append(m_filter.filter(sample));
    const core::Stroke finished = std::move(*m_activeStroke);
    m_activeStroke.reset();

    std::vector<InkVertex>& into = activeVertices();
    into.resize(m_activeStrokeFirstVertex);
    StrokeMesh mesh{
        .id = finished.id(),
        .vertices = {},
        .translucent = m_activeIsTranslucent,
    };
    core::appendStroke(mesh.vertices, finished);
    into.insert(into.end(), mesh.vertices.begin(), mesh.vertices.end());
    m_meshes.push_back(std::move(mesh));
    ++m_generation;
    if (m_sink != nullptr) {
        m_sink->strokeFinished(sample);
        m_sink->strokeCompleted(finished);
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
