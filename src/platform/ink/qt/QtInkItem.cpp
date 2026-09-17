#include "platform/ink/qt/QtInkItem.hpp"

#include <rhi/qrhi.h>

#include <QFile>
#include <QLineF>
#include <QMatrix4x4>
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
constexpr quint32 kInitialVertexBufferBytes = 64U * 1024U;
constexpr quint32 kMatrixBytes = 64;
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

[[nodiscard]] QShader loadShader(const QString& path) {
    QFile file{path};
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

class QtInkRenderer final : public QQuickRhiItemRenderer {
public:
    void initialize(QRhiCommandBuffer* commandBuffer) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* commandBuffer) override;

private:
    void uploadVertices(QRhiResourceUpdateBatch& updates);

    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    int m_sampleCount{0};

    std::vector<InkVertex> m_vertices;
    std::size_t m_uploadedVertexCount{0};
    std::uint64_t m_generation{0};
    float m_logicalWidth{0.0F};
    float m_logicalHeight{0.0F};
    core::Viewport m_viewport;
};

void QtInkRenderer::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    if (m_pipeline && m_sampleCount == renderTarget()->sampleCount()) {
        return;
    }
    m_sampleCount = renderTarget()->sampleCount();
    QRhi* const device = rhi();

    if (!m_uniformBuffer) {
        m_uniformBuffer.reset(
            device->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kMatrixBytes));
        m_uniformBuffer->create();
    }
    if (!m_bindings) {
        m_bindings.reset(device->newShaderResourceBindings());
        m_bindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                     m_uniformBuffer.get()),
        });
        m_bindings->create();
    }

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding{static_cast<quint32>(sizeof(InkVertex))}});
    inputLayout.setAttributes({
        QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2,
                                 static_cast<quint32>(offsetof(InkVertex, x))},
        QRhiVertexInputAttribute{0, 1, QRhiVertexInputAttribute::Float4,
                                 static_cast<quint32>(offsetof(InkVertex, red))},
    });

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;

    m_pipeline.reset(device->newGraphicsPipeline());
    m_pipeline->setShaderStages({
        QRhiShaderStage{QRhiShaderStage::Vertex,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/ink.vert.qsb"))},
        QRhiShaderStage{QRhiShaderStage::Fragment,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/ink.frag.qsb"))},
    });
    m_pipeline->setTargetBlends({blend});
    m_pipeline->setSampleCount(m_sampleCount);
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_bindings.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_pipeline->create();
}

void QtInkRenderer::synchronize(QQuickRhiItem* item) {
    const auto* const inkItem = qobject_cast<QtInkItem*>(item);
    if (inkItem == nullptr) {
        return;
    }

    const std::vector<InkVertex>& source = inkItem->vertices();
    if (inkItem->generation() != m_generation || source.size() < m_vertices.size()) {
        m_generation = inkItem->generation();
        m_vertices.clear();
        m_uploadedVertexCount = 0;
    }
    const auto newVertices = std::span{source}.subspan(m_vertices.size());
    m_vertices.insert(m_vertices.end(), newVertices.begin(), newVertices.end());

    m_logicalWidth = static_cast<float>(inkItem->width());
    m_logicalHeight = static_cast<float>(inkItem->height());
    m_viewport = inkItem->viewport();
}

void QtInkRenderer::uploadVertices(QRhiResourceUpdateBatch& updates) {
    const auto requiredBytes = static_cast<quint32>(m_vertices.size() * sizeof(InkVertex));
    if (requiredBytes == 0) {
        return;
    }

    if (!m_vertexBuffer || m_vertexBuffer->size() < requiredBytes) {
        quint32 capacity = m_vertexBuffer ? m_vertexBuffer->size() : kInitialVertexBufferBytes;
        while (capacity < requiredBytes) {
            capacity *= 2U;
        }
        m_vertexBuffer.reset(
            rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, capacity));
        m_vertexBuffer->create();
        m_uploadedVertexCount = 0;
    }

    if (m_uploadedVertexCount < m_vertices.size()) {
        const auto pending = std::span{m_vertices}.subspan(m_uploadedVertexCount);
        const auto offset = static_cast<quint32>(m_uploadedVertexCount * sizeof(InkVertex));
        updates.updateDynamicBuffer(m_vertexBuffer.get(), offset,
                                    static_cast<quint32>(pending.size_bytes()), pending.data());
        m_uploadedVertexCount = m_vertices.size();
    }
}

void QtInkRenderer::render(QRhiCommandBuffer* commandBuffer) {
    QRhiResourceUpdateBatch* const updates = rhi()->nextResourceUpdateBatch();
    uploadVertices(*updates);

    QMatrix4x4 projection = rhi()->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);
    projection.scale(m_viewport.scale());
    projection.translate(-m_viewport.origin().x, -m_viewport.origin().y);
    updates->updateDynamicBuffer(m_uniformBuffer.get(), 0, kMatrixBytes, projection.constData());

    commandBuffer->beginPass(renderTarget(), Qt::white, {1.0F, 0}, updates);
    const QSize outputSize = renderTarget()->pixelSize();
    commandBuffer->setViewport(QRhiViewport{0.0F, 0.0F, static_cast<float>(outputSize.width()),
                                            static_cast<float>(outputSize.height())});

    if (!m_vertices.empty()) {
        commandBuffer->setGraphicsPipeline(m_pipeline.get());
        commandBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexInput{m_vertexBuffer.get(), 0};
        commandBuffer->setVertexInput(0, 1, &vertexInput);
        commandBuffer->draw(static_cast<quint32>(m_vertices.size()));
    }
    commandBuffer->endPass();
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
    m_vertices.clear();

    const bool otherPage = page.id() != m_shownPage;
    const bool otherPaper =
        style.paper != m_pageStyle.paper || style.orientation != m_pageStyle.orientation;
    m_shownPage = page.id();
    m_pageStyle = style;
    if (otherPage || otherPaper || !m_viewFitted) {
        fitPage();
    }

    for (const core::PlacedStroke& placed : page.strokes()) {
        if (std::ranges::find(hidden, placed.stroke.id()) != hidden.end()) {
            continue;
        }
        core::appendStroke(m_vertices, placed.stroke);
    }

    ++m_generation;
    update();
}

void QtInkItem::clear() {
    cancelStroke();
    m_vertices.clear();
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

QPointF QtInkItem::viewOrigin() const noexcept {
    return {m_viewport.origin().x, m_viewport.origin().y};
}

core::ViewSize QtInkItem::viewSize() const noexcept {
    return {.width = static_cast<float>(width()), .height = static_cast<float>(height())};
}

void QtInkItem::changeView(const core::Viewport& viewport) {
    core::Viewport kept = viewport;
    kept.keepPaperInView(viewSize(), core::paperSize(m_pageStyle.paper, m_pageStyle.orientation));
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
    viewport.fit(viewSize(), core::paperSize(m_pageStyle.paper, m_pageStyle.orientation));
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

void QtInkItem::press(const InkSample& sample, bool eraserTip) {
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
        m_sink->eraserMoved(sample, sample);
    }
}

void QtInkItem::moveEraser(const InkSample& sample) {
    if (!m_eraserPosition) {
        return;
    }
    const InkSample from = std::exchange(*m_eraserPosition, sample);
    if (m_sink != nullptr) {
        m_sink->eraserMoved(from, sample);
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

void QtInkItem::beginStroke(const InkSample& sample) {
    finishErase();
    cancelStroke();
    m_filter.reset();
    m_activeStroke.emplace(m_ids.next(), m_style);
    m_activeStroke->append(m_filter.filter(sample));
    m_activeStrokeFirstVertex = m_vertices.size();
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
    core::appendSegment(m_vertices, previous, smoothed, m_activeStroke->style());
    if (m_sink != nullptr) {
        m_sink->sampleAdded(sample);
    }
    update();
}

void QtInkItem::endStroke(const InkSample& sample) {
    if (!m_activeStroke) {
        return;
    }
    const InkSample previous = m_activeStroke->samples().back();
    const InkSample smoothed = m_filter.filter(sample);
    m_activeStroke->append(smoothed);
    core::appendSegment(m_vertices, previous, smoothed, m_activeStroke->style());

    const core::Stroke finished = std::move(*m_activeStroke);
    m_activeStroke.reset();
    m_vertices.resize(m_activeStrokeFirstVertex);
    core::appendStroke(m_vertices, finished);
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
    m_vertices.resize(m_activeStrokeFirstVertex);
    ++m_generation;
    if (m_sink != nullptr) {
        m_sink->strokeCancelled();
    }
    update();
}

}
