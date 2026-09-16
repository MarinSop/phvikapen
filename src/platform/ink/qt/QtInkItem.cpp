#include "platform/ink/qt/QtInkItem.hpp"

#include <rhi/qrhi.h>

#include <QFile>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QTabletEvent>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace phvikapen::platform::ink {
namespace {

using core::InkSample;
using core::InkVertex;

constexpr std::chrono::microseconds::rep kMicrosecondsPerMillisecond = 1000;
constexpr quint32 kInitialVertexBufferBytes = 64U * 1024U;
constexpr quint32 kMatrixBytes = 64;

[[nodiscard]] InkSample makeSample(const QPointF& position, qreal pressure, qreal tiltX,
                                   qreal tiltY, quint64 timestampMs) {
    // Qt reports event timestamps in milliseconds.
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

[[nodiscard]] InkSample makeSample(const QMouseEvent& event) {
    // Mice have no pressure sensor; draw at full pressure.
    return makeSample(event.position(), 1.0, 0.0, 0.0, event.timestamp());
}

[[nodiscard]] QShader loadShader(const QString& path) {
    QFile file{path};
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

/// Draws the tessellated wet ink of a QtInkItem on the render thread.
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

    // Shaders output premultiplied alpha; the default blend factors match that.
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
    // Only vertices added since the last frame are copied.
    const auto newVertices = std::span{source}.subspan(m_vertices.size());
    m_vertices.insert(m_vertices.end(), newVertices.begin(), newVertices.end());

    m_logicalWidth = static_cast<float>(inkItem->width());
    m_logicalHeight = static_cast<float>(inkItem->height());
}

void QtInkRenderer::uploadVertices(QRhiResourceUpdateBatch& updates) {
    const auto requiredBytes = static_cast<quint32>(m_vertices.size() * sizeof(InkVertex));
    if (requiredBytes == 0) {
        return;
    }

    if (!m_vertexBuffer || m_vertexBuffer->size() < requiredBytes) {
        // Grow geometrically so that appending samples stays amortized O(1).
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

    // Page coordinates are logical pixels with y pointing down.
    QMatrix4x4 projection = rhi()->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);
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

} // namespace

QtInkItem::QtInkItem(QQuickItem* parent) : QQuickRhiItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    connect(this, &QQuickItem::windowChanged, this, &QtInkItem::observeWindow);
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

void QtInkItem::setStrokes(std::vector<core::Stroke> strokes) {
    cancelStroke();
    m_strokes = std::move(strokes);
    m_vertices.clear();

    for (const core::Stroke& stroke : m_strokes) {
        const std::span<const InkSample> samples = stroke.samples();
        if (samples.empty()) {
            continue;
        }
        if (samples.size() == 1) {
            // A tap was stored as a single sample and is drawn as a dot.
            core::appendSegment(m_vertices, samples.front(), samples.front(), stroke.style());
            continue;
        }
        for (std::size_t i = 1; i < samples.size(); ++i) {
            core::appendSegment(m_vertices, samples[i - 1], samples[i], stroke.style());
        }
    }

    ++m_generation;
    update();
}

void QtInkItem::clear() {
    cancelStroke();
    m_strokes.clear();
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

QQuickRhiItemRenderer* QtInkItem::createRenderer() {
    // Ownership passes to Qt Quick, which destroys the renderer on the render thread.
    return new QtInkRenderer;
}

void QtInkItem::mousePressEvent(QMouseEvent* event) {
    beginStroke(makeSample(*event));
    event->accept();
}

void QtInkItem::mouseMoveEvent(QMouseEvent* event) {
    if (m_activeStroke) {
        appendToStroke(makeSample(*event));
    }
    event->accept();
}

void QtInkItem::mouseReleaseEvent(QMouseEvent* event) {
    if (m_activeStroke) {
        endStroke(makeSample(*event));
    }
    event->accept();
}

void QtInkItem::mouseUngrabEvent() {
    cancelStroke();
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
    const InkSample sample =
        makeSample(position, event.pressure(), event.xTilt(), event.yTilt(), event.timestamp());

    switch (event.type()) {
    case QEvent::TabletPress:
        if (!isVisible() || !isEnabled() || !contains(position)) {
            return false;
        }
        beginStroke(sample);
        break;
    case QEvent::TabletMove:
        if (!m_activeStroke) {
            return false;
        }
        appendToStroke(sample);
        break;
    case QEvent::TabletRelease:
        if (!m_activeStroke) {
            return false;
        }
        endStroke(sample);
        break;
    default:
        return false;
    }

    // Accepted tablet events are not converted into mouse events.
    event.accept();
    return true;
}

void QtInkItem::beginStroke(const InkSample& sample) {
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
    // A tap without movement becomes a dot.
    core::appendSegment(m_vertices, previous, smoothed, m_activeStroke->style());

    m_strokes.push_back(std::move(*m_activeStroke));
    m_activeStroke.reset();
    if (m_sink != nullptr) {
        m_sink->strokeFinished(sample);
        m_sink->strokeCompleted(m_strokes.back());
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

} // namespace phvikapen::platform::ink
