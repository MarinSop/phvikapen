#include "platform/ink/qt/QtInkRenderer.hpp"

#include "core/geometry/Viewport.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "core/model/PageStyle.hpp"
#include "platform/ink/qt/QtInkItem.hpp"

#include <rhi/qrhi.h>

#include <QFile>
#include <QMatrix4x4>

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::platform::ink {
namespace {

using core::InkVertex;

constexpr quint32 kInitialVertexBufferBytes = 64U * 1024U;
constexpr quint32 kMatrixBytes = 64;
constexpr std::size_t kBackgroundUniformCount = 48;
constexpr std::array<float, 12> kBackgroundCorners{
    0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F, 1.0F,
};

constexpr float kLineWidth = 1.0F;
constexpr float kDotRadius = 1.25F;
constexpr float kLinedTopMargin = core::millimeters(20.0F);
constexpr float kLinedLeftMargin = core::millimeters(25.0F);

constexpr std::initializer_list<float> kDeskColor{0.89F, 0.90F, 0.91F, 1.0F};
constexpr std::initializer_list<float> kPaperColor{1.0F, 1.0F, 1.0F, 1.0F};
constexpr std::initializer_list<float> kRuleColor{0.70F, 0.80F, 0.91F, 1.0F};
constexpr std::initializer_list<float> kGridColor{0.82F, 0.86F, 0.91F, 1.0F};
constexpr std::initializer_list<float> kDotColor{0.60F, 0.65F, 0.72F, 1.0F};
constexpr std::initializer_list<float> kMarginColor{0.93F, 0.60F, 0.60F, 1.0F};

[[nodiscard]] QShader loadShader(const QString& path) {
    QFile file{path};
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

[[nodiscard]] std::initializer_list<float> patternColor(core::Background background) {
    switch (background) {
    case core::Background::Grid:
        return kGridColor;
    case core::Background::Dotted:
        return kDotColor;
    case core::Background::Blank:
    case core::Background::Lined:
        return kRuleColor;
    }
    return kRuleColor;
}

}

QtInkRenderer::QtInkRenderer() = default;

QtInkRenderer::~QtInkRenderer() = default;

void QtInkRenderer::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    if (m_pipeline && m_sampleCount == renderTarget()->sampleCount()) {
        return;
    }
    m_sampleCount = renderTarget()->sampleCount();
    createInkPipeline();
    createBackgroundPipeline();
}

void QtInkRenderer::createInkPipeline() {
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
    m_pipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
    m_pipeline->setTargetBlends({blend});
    m_pipeline->setSampleCount(m_sampleCount);
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_bindings.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_pipeline->create();
}

void QtInkRenderer::createBackgroundPipeline() {
    QRhi* const device = rhi();
    if (!m_backgroundVertices) {
        m_backgroundVertices.reset(
            device->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                              static_cast<quint32>(std::span{kBackgroundCorners}.size_bytes())));
        m_backgroundVertices->create();
        m_backgroundVerticesUploaded = false;
    }
    if (!m_backgroundUniforms) {
        m_backgroundUniforms.reset(
            device->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                              static_cast<quint32>(kBackgroundUniformCount * sizeof(float))));
        m_backgroundUniforms->create();
    }
    if (!m_backgroundBindings) {
        m_backgroundBindings.reset(device->newShaderResourceBindings());
        m_backgroundBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0,
                                                     QRhiShaderResourceBinding::VertexStage
                                                         | QRhiShaderResourceBinding::FragmentStage,
                                                     m_backgroundUniforms.get()),
        });
        m_backgroundBindings->create();
    }

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding{2U * sizeof(float)}});
    inputLayout.setAttributes({
        QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2, 0},
    });

    m_backgroundPipeline.reset(device->newGraphicsPipeline());
    m_backgroundPipeline->setShaderStages({
        QRhiShaderStage{
            QRhiShaderStage::Vertex,
            loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/background.vert.qsb"))},
        QRhiShaderStage{
            QRhiShaderStage::Fragment,
            loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/background.frag.qsb"))},
    });
    m_backgroundPipeline->setSampleCount(m_sampleCount);
    m_backgroundPipeline->setVertexInputLayout(inputLayout);
    m_backgroundPipeline->setShaderResourceBindings(m_backgroundBindings.get());
    m_backgroundPipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_backgroundPipeline->create();
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
    m_pageStyle = inkItem->pageStyle();
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

void QtInkRenderer::updateBackground(QRhiResourceUpdateBatch& updates) {
    if (!m_backgroundVerticesUploaded) {
        updates.uploadStaticBuffer(m_backgroundVertices.get(), kBackgroundCorners.data());
        m_backgroundVerticesUploaded = true;
    }

    const float pixelRatio =
        m_logicalWidth > 0.0F
            ? static_cast<float>(renderTarget()->pixelSize().width()) / m_logicalWidth
            : 1.0F;
    const std::optional<core::PaperSize> paper =
        core::paperSize(m_pageStyle.paper, m_pageStyle.orientation);
    const bool lined = m_pageStyle.background == core::Background::Lined;
    const float lineWidth = kLineWidth * pixelRatio;
    const float dotRadius = kDotRadius * pixelRatio;

    QMatrix4x4 projection = rhi()->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);

    std::array<float, kBackgroundUniformCount> uniforms{};
    auto* out = uniforms.begin();
    const auto put = [&out](std::initializer_list<float> values) {
        out = std::ranges::copy(values, out).out;
    };
    out = std::copy_n(projection.constData(), kMatrixBytes / sizeof(float), out);
    put({m_viewport.origin().x, m_viewport.origin().y, m_viewport.scale(), pixelRatio});
    put({m_logicalWidth, m_logicalHeight, 0.0F, 0.0F});
    put({
        paper ? paper->width : 0.0F,
        paper ? paper->height : 0.0F,
        paper ? 1.0F : 0.0F,
        paper && lined ? kLinedLeftMargin : -1.0F,
    });
    put({
        static_cast<float>(m_pageStyle.background),
        m_pageStyle.spacing,
        m_pageStyle.background == core::Background::Dotted ? dotRadius : lineWidth,
        paper && lined ? kLinedTopMargin : 0.0F,
    });
    put(kDeskColor);
    put(kPaperColor);
    put(patternColor(m_pageStyle.background));
    put(kMarginColor);

    updates.updateDynamicBuffer(m_backgroundUniforms.get(), 0,
                                static_cast<quint32>(std::span{uniforms}.size_bytes()),
                                uniforms.data());
}

QRhiScissor QtInkRenderer::inkScissor(const QSize& outputSize) const {
    const std::optional<core::PaperSize> paper =
        core::paperSize(m_pageStyle.paper, m_pageStyle.orientation);
    if (!paper) {
        return QRhiScissor{0, 0, outputSize.width(), outputSize.height()};
    }
    const float pixelRatio =
        m_logicalWidth > 0.0F ? static_cast<float>(outputSize.width()) / m_logicalWidth : 1.0F;
    const core::Point topLeft = m_viewport.toView({.x = 0.0F, .y = 0.0F});
    const core::Point bottomRight = m_viewport.toView({.x = paper->width, .y = paper->height});
    const int left = std::clamp(static_cast<int>(topLeft.x * pixelRatio), 0, outputSize.width());
    const int right =
        std::clamp(static_cast<int>(bottomRight.x * pixelRatio), 0, outputSize.width());
    const int top = std::clamp(static_cast<int>(topLeft.y * pixelRatio), 0, outputSize.height());
    const int bottom =
        std::clamp(static_cast<int>(bottomRight.y * pixelRatio), 0, outputSize.height());
    return QRhiScissor{left, outputSize.height() - bottom, right - left, bottom - top};
}

void QtInkRenderer::render(QRhiCommandBuffer* commandBuffer) {
    QRhiResourceUpdateBatch* const updates = rhi()->nextResourceUpdateBatch();
    uploadVertices(*updates);
    updateBackground(*updates);

    QMatrix4x4 projection = rhi()->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);
    projection.scale(m_viewport.scale());
    projection.translate(-m_viewport.origin().x, -m_viewport.origin().y);
    updates->updateDynamicBuffer(m_uniformBuffer.get(), 0, kMatrixBytes, projection.constData());

    commandBuffer->beginPass(renderTarget(), Qt::white, {1.0F, 0}, updates);
    const QSize outputSize = renderTarget()->pixelSize();
    commandBuffer->setViewport(QRhiViewport{0.0F, 0.0F, static_cast<float>(outputSize.width()),
                                            static_cast<float>(outputSize.height())});

    commandBuffer->setGraphicsPipeline(m_backgroundPipeline.get());
    commandBuffer->setShaderResources();
    const QRhiCommandBuffer::VertexInput backgroundInput{m_backgroundVertices.get(), 0};
    commandBuffer->setVertexInput(0, 1, &backgroundInput);
    commandBuffer->draw(static_cast<quint32>(kBackgroundCorners.size() / 2));

    if (!m_vertices.empty()) {
        commandBuffer->setGraphicsPipeline(m_pipeline.get());
        commandBuffer->setScissor(inkScissor(outputSize));
        commandBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexInput{m_vertexBuffer.get(), 0};
        commandBuffer->setVertexInput(0, 1, &vertexInput);
        commandBuffer->draw(static_cast<quint32>(m_vertices.size()));
    }
    commandBuffer->endPass();
}

}
