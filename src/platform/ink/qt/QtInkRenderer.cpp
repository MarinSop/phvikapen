#include "platform/ink/qt/QtInkRenderer.hpp"

#include "core/geometry/Viewport.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "core/model/PageStyle.hpp"
#include "platform/ink/qt/QtInkItem.hpp"
#include "platform/render/PaperLook.hpp"

#include <rhi/qrhi.h>

#include <QFile>
#include <QImage>
#include <QMatrix4x4>
#include <QSize>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::platform::ink {
namespace {

using core::InkVertex;

constexpr quint32 kInitialVertexBufferBytes = 64U * 1024U;
constexpr quint32 kMatrixBytes = 64;
constexpr quint32 kVectorBytes = 16;
constexpr std::size_t kMostSheets = 16;
constexpr std::size_t kFloatsPerVector = 4;
constexpr std::size_t kBackgroundUniformCount = 48 + (2 * kFloatsPerVector * kMostSheets);
constexpr std::array<float, 12> kBackgroundCorners{
    0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F, 1.0F,
};

[[nodiscard]] QShader loadShader(const QString& path) {
    QFile file{path};
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

}

QtInkRenderer::QtInkRenderer() = default;

QtInkRenderer::~QtInkRenderer() = default;

void QtInkRenderer::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    if (m_pipeline && m_sampleCount == renderTarget()->sampleCount()) {
        return;
    }
    m_sampleCount = renderTarget()->sampleCount();
    m_compositePipeline.reset();
    m_layerPipeline.reset();
    m_layerTarget.reset();
    m_layerPass.reset();
    m_layerSamples.reset();
    m_layerTexture.reset();
    createInkPipeline();
    createBackgroundPipeline();
    createMediaPipeline();
    createLayerPipeline();
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

void QtInkRenderer::createLayerPipeline() {
    QRhi* const device = rhi();
    if (!m_layerSampler) {
        m_layerSampler.reset(device->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest,
                                                QRhiSampler::None, QRhiSampler::ClampToEdge,
                                                QRhiSampler::ClampToEdge));
        m_layerSampler->create();
    }
    if (!m_layerUniforms) {
        m_layerUniforms.reset(
            device->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kVectorBytes));
        m_layerUniforms->create();
    }

    updateLayerTarget();

    // Translucent ink is drawn once into its own picture, so a stroke crossing itself
    // does not darken where it overlaps.
    QRhiGraphicsPipeline::TargetBlend layerBlend;
    layerBlend.enable = true;
    layerBlend.srcColor = QRhiGraphicsPipeline::One;
    layerBlend.dstColor = QRhiGraphicsPipeline::One;
    layerBlend.opColor = QRhiGraphicsPipeline::Max;
    layerBlend.srcAlpha = QRhiGraphicsPipeline::One;
    layerBlend.dstAlpha = QRhiGraphicsPipeline::One;
    layerBlend.opAlpha = QRhiGraphicsPipeline::Max;

    QRhiVertexInputLayout inkLayout;
    inkLayout.setBindings({QRhiVertexInputBinding{static_cast<quint32>(sizeof(InkVertex))}});
    inkLayout.setAttributes({
        QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2,
                                 static_cast<quint32>(offsetof(InkVertex, x))},
        QRhiVertexInputAttribute{0, 1, QRhiVertexInputAttribute::Float4,
                                 static_cast<quint32>(offsetof(InkVertex, red))},
    });

    m_layerPipeline.reset(device->newGraphicsPipeline());
    m_layerPipeline->setShaderStages({
        QRhiShaderStage{QRhiShaderStage::Vertex,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/ink.vert.qsb"))},
        QRhiShaderStage{QRhiShaderStage::Fragment,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/ink.frag.qsb"))},
    });
    m_layerPipeline->setTargetBlends({layerBlend});
    m_layerPipeline->setSampleCount(m_sampleCount);
    m_layerPipeline->setVertexInputLayout(inkLayout);
    m_layerPipeline->setShaderResourceBindings(m_bindings.get());
    m_layerPipeline->setRenderPassDescriptor(m_layerPass.get());
    m_layerPipeline->create();

    QRhiGraphicsPipeline::TargetBlend compositeBlend;
    compositeBlend.enable = true;
    compositeBlend.srcColor = QRhiGraphicsPipeline::One;
    compositeBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    compositeBlend.srcAlpha = QRhiGraphicsPipeline::One;
    compositeBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    QRhiVertexInputLayout cornerLayout;
    cornerLayout.setBindings({QRhiVertexInputBinding{2U * sizeof(float)}});
    cornerLayout.setAttributes({
        QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2, 0},
    });

    m_compositePipeline.reset(device->newGraphicsPipeline());
    m_compositePipeline->setShaderStages({
        QRhiShaderStage{QRhiShaderStage::Vertex,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/layer.vert.qsb"))},
        QRhiShaderStage{QRhiShaderStage::Fragment,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/layer.frag.qsb"))},
    });
    m_compositePipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
    m_compositePipeline->setTargetBlends({compositeBlend});
    m_compositePipeline->setSampleCount(m_sampleCount);
    m_compositePipeline->setVertexInputLayout(cornerLayout);
    m_compositePipeline->setShaderResourceBindings(m_layerBindings.get());
    m_compositePipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_compositePipeline->create();
}

void QtInkRenderer::updateLayerTarget() {
    QRhi* const device = rhi();
    const QSize wanted = renderTarget()->pixelSize();
    if (wanted.isEmpty() || (m_layerTexture && m_layerTexture->pixelSize() == wanted)) {
        return;
    }

    m_layerTexture.reset(
        device->newTexture(QRhiTexture::RGBA8, wanted, 1, QRhiTexture::RenderTarget));
    m_layerTexture->create();

    // The layer is drawn with as many samples to a pixel as the window, and resolved for blending.
    QRhiColorAttachment colour;
    if (m_sampleCount > 1) {
        m_layerSamples.reset(device->newRenderBuffer(QRhiRenderBuffer::Color, wanted, m_sampleCount,
                                                     {}, m_layerTexture->format()));
        m_layerSamples->create();
        colour.setRenderBuffer(m_layerSamples.get());
        colour.setResolveTexture(m_layerTexture.get());
    } else {
        m_layerSamples.reset();
        colour.setTexture(m_layerTexture.get());
    }
    m_layerTarget.reset(device->newTextureRenderTarget(QRhiTextureRenderTargetDescription{colour}));
    if (!m_layerPass) {
        m_layerPass.reset(m_layerTarget->newCompatibleRenderPassDescriptor());
    }
    m_layerTarget->setRenderPassDescriptor(m_layerPass.get());
    m_layerTarget->create();

    if (!m_layerBindings) {
        m_layerBindings.reset(device->newShaderResourceBindings());
    }
    m_layerBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_layerUniforms.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                  m_layerTexture.get(), m_layerSampler.get()),
    });
    m_layerBindings->create();
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

void QtInkRenderer::createMediaPipeline() {
    QRhi* const device = rhi();
    if (!m_mediaUniforms) {
        m_mediaUniforms.reset(device->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                kMatrixBytes + kVectorBytes));
        m_mediaUniforms->create();
    }
    if (!m_mediaSampler) {
        m_mediaSampler.reset(device->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                                QRhiSampler::Linear, QRhiSampler::ClampToEdge,
                                                QRhiSampler::ClampToEdge));
        m_mediaSampler->create();
    }
    if (!m_mediaTexture) {
        m_mediaTexture.reset(device->newTexture(QRhiTexture::RGBA8, QSize{1, 1}));
        m_mediaTexture->create();
    }
    if (!m_mediaBindings) {
        m_mediaBindings.reset(device->newShaderResourceBindings());
        m_mediaBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                     m_mediaUniforms.get()),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      m_mediaTexture.get(), m_mediaSampler.get()),
        });
        m_mediaBindings->create();
    }

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding{2U * sizeof(float)}});
    inputLayout.setAttributes({
        QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2, 0},
    });

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;

    m_mediaPipeline.reset(device->newGraphicsPipeline());
    m_mediaPipeline->setShaderStages({
        QRhiShaderStage{QRhiShaderStage::Vertex,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/media.vert.qsb"))},
        QRhiShaderStage{QRhiShaderStage::Fragment,
                        loadShader(QStringLiteral(":/phvikapen/ink/qt/shaders/media.frag.qsb"))},
    });
    m_mediaPipeline->setTargetBlends({blend});
    m_mediaPipeline->setSampleCount(m_sampleCount);
    m_mediaPipeline->setVertexInputLayout(inputLayout);
    m_mediaPipeline->setShaderResourceBindings(m_mediaBindings.get());
    m_mediaPipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_mediaPipeline->create();
}

void QtInkRenderer::bindMedia(MediaEntry& entry) {
    entry.bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                 entry.uniforms.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                  entry.texture.get(), m_mediaSampler.get()),
    });
    entry.bindings->create();
}

void QtInkRenderer::updateMedia(QRhiResourceUpdateBatch& updates) {
    QRhi* const device = rhi();
    QMatrix4x4 projection = device->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);
    projection.scale(m_viewport.scale());
    projection.translate(-m_viewport.origin().x, -m_viewport.origin().y);

    for (MediaEntry& entry : m_media) {
        if (entry.picture.isNull() || entry.area.isEmpty()) {
            continue;
        }
        if (!entry.uniforms) {
            entry.uniforms.reset(device->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                   kMatrixBytes + kVectorBytes));
            entry.uniforms->create();
        }
        if (!entry.texture) {
            entry.texture.reset(device->newTexture(QRhiTexture::RGBA8, QSize{1, 1}));
            entry.texture->create();
            entry.uploaded = false;
        }
        if (!entry.bindings) {
            entry.bindings.reset(device->newShaderResourceBindings());
            bindMedia(entry);
        }

        std::array<float, (kMatrixBytes + kVectorBytes) / sizeof(float)> uniforms{};
        std::copy_n(projection.constData(), kMatrixBytes / sizeof(float), uniforms.begin());
        uniforms.at(kMatrixBytes / sizeof(float)) = static_cast<float>(entry.area.x());
        uniforms.at((kMatrixBytes / sizeof(float)) + 1) = static_cast<float>(entry.area.y());
        uniforms.at((kMatrixBytes / sizeof(float)) + 2) = static_cast<float>(entry.area.width());
        uniforms.at((kMatrixBytes / sizeof(float)) + 3) = static_cast<float>(entry.area.height());
        updates.updateDynamicBuffer(entry.uniforms.get(), 0,
                                    static_cast<quint32>(std::span{uniforms}.size_bytes()),
                                    uniforms.data());

        if (entry.uploaded) {
            continue;
        }
        const QImage image = entry.picture.convertToFormat(QImage::Format_RGBA8888);
        if (entry.texture->pixelSize() != image.size()) {
            entry.texture->setPixelSize(image.size());
            entry.texture->create();
            bindMedia(entry);
        }
        updates.uploadTexture(entry.texture.get(), image);
        entry.uploaded = true;
    }
}

void QtInkRenderer::synchronize(QQuickRhiItem* item) {
    const auto* const inkItem = qobject_cast<QtInkItem*>(item);
    if (inkItem == nullptr) {
        return;
    }

    const std::vector<InkVertex>& ink = inkItem->vertices();
    const std::vector<InkVertex>& highlights = inkItem->highlights();
    const std::vector<InkVertex>& overlay = inkItem->overlay();
    if (inkItem->generation() != m_generation || ink.size() < m_ink.data.size()
        || highlights.size() < m_highlights.data.size() || overlay.size() < m_overlay.data.size()) {
        m_generation = inkItem->generation();
        m_ink.data.clear();
        m_ink.uploaded = 0;
        m_highlights.data.clear();
        m_highlights.uploaded = 0;
        m_overlay.data.clear();
        m_overlay.uploaded = 0;
    }
    const auto grown = [](Stream& stream, const std::vector<InkVertex>& source) {
        const auto added = std::span{source}.subspan(stream.data.size());
        stream.data.insert(stream.data.end(), added.begin(), added.end());
    };
    grown(m_ink, ink);
    grown(m_highlights, highlights);
    grown(m_overlay, overlay);

    m_logicalWidth = static_cast<float>(inkItem->width());
    m_logicalHeight = static_cast<float>(inkItem->height());
    m_viewport = inkItem->viewport();
    m_pageStyle = inkItem->pageStyle();
    m_deskColor = inkItem->deskColor();
    m_sheets = inkItem->visibleSheets();
    if (inkItem->mediaGeneration() != m_mediaGeneration) {
        m_mediaGeneration = inkItem->mediaGeneration();
        std::vector<MediaEntry> kept;
        kept.reserve(inkItem->mediaDraws().size());
        for (const QtInkItem::MediaDraw& draw : inkItem->mediaDraws()) {
            const auto found = std::ranges::find_if(m_media, [&draw](const MediaEntry& entry) {
                return entry.picture.cacheKey() == draw.picture.cacheKey();
            });
            if (found != m_media.end()) {
                found->area = draw.area;
                kept.push_back(std::move(*found));
                continue;
            }
            MediaEntry entry;
            entry.picture = draw.picture;
            entry.area = draw.area;
            kept.push_back(std::move(entry));
        }
        m_media = std::move(kept);
    } else {
        const std::vector<QtInkItem::MediaDraw>& draws = inkItem->mediaDraws();
        for (std::size_t i = 0; i < m_media.size() && i < draws.size(); ++i) {
            m_media[i].area = draws[i].area;
        }
    }
}

void QtInkRenderer::uploadStream(Stream& stream, QRhiResourceUpdateBatch& updates) {
    const auto requiredBytes = static_cast<quint32>(stream.data.size() * sizeof(InkVertex));
    if (requiredBytes == 0) {
        return;
    }

    if (!stream.buffer || stream.buffer->size() < requiredBytes) {
        quint32 capacity = stream.buffer ? stream.buffer->size() : kInitialVertexBufferBytes;
        while (capacity < requiredBytes) {
            capacity *= 2U;
        }
        stream.buffer.reset(
            rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, capacity));
        stream.buffer->create();
        stream.uploaded = 0;
    }

    if (stream.uploaded < stream.data.size()) {
        const auto pending = std::span{stream.data}.subspan(stream.uploaded);
        const auto offset = static_cast<quint32>(stream.uploaded * sizeof(InkVertex));
        updates.updateDynamicBuffer(stream.buffer.get(), offset,
                                    static_cast<quint32>(pending.size_bytes()), pending.data());
        stream.uploaded = stream.data.size();
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
    const std::optional<core::PaperSize> paper = core::paperSize(m_pageStyle);
    const bool lined = m_pageStyle.background == core::Background::Lined;
    const float ruleWidth = render::lineWidthOf(m_pageStyle);
    const float lineWidth = ruleWidth * pixelRatio;
    const float dotRadius = ruleWidth * render::kDotsPerRule * pixelRatio;

    QMatrix4x4 projection = rhi()->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);

    std::array<float, kBackgroundUniformCount> uniforms{};
    auto* out = uniforms.data();
    const auto put = [&out](std::span<const float> values) {
        out = std::ranges::copy(values, out).out;
    };
    out = std::copy_n(projection.constData(), kMatrixBytes / sizeof(float), out);
    put(std::array{m_viewport.origin().x, m_viewport.origin().y, m_viewport.scale(), pixelRatio});
    put(std::array{m_logicalWidth, m_logicalHeight, static_cast<float>(m_sheets.size()), 0.0F});
    put(std::array{
        paper ? paper->width : 0.0F,
        paper ? paper->height : 0.0F,
        paper ? 1.0F : 0.0F,
        paper && lined && m_pageStyle.margin ? m_pageStyle.marginAt : -1.0F,
    });
    put(std::array{
        static_cast<float>(m_pageStyle.background),
        m_pageStyle.spacing,
        m_pageStyle.background == core::Background::Dotted ? dotRadius : lineWidth,
        paper && lined ? render::kLinedTopMargin : 0.0F,
    });
    put(std::array{m_deskColor.redF(), m_deskColor.greenF(), m_deskColor.blueF(), 1.0F});
    put(render::paperColorOf(m_pageStyle));
    put(render::lineColorOf(m_pageStyle));
    put(render::marginColorOf(m_pageStyle));

    // The sheets in view come first, then how each of them is ruled; the rest of the room is
    // left empty.
    std::array<float, kFloatsPerVector * kMostSheets> rulings{};
    std::size_t at = 0;
    for (const QtInkItem::VisibleSheet& sheet : m_sheets) {
        put(std::array{sheet.area.left, sheet.area.top, sheet.area.width(), sheet.area.height()});
        const float sheetWidth = render::lineWidthOf(sheet.style) * pixelRatio;
        rulings.at(at++) = static_cast<float>(sheet.style.background);
        rulings.at(at++) = sheet.style.spacing;
        rulings.at(at++) = sheet.style.background == core::Background::Dotted
                               ? sheetWidth * render::kDotsPerRule
                               : sheetWidth;
        rulings.at(at++) =
            sheet.style.background == core::Background::Lined && sheet.style.margin ? 1.0F : 0.0F;
    }
    std::advance(out, kFloatsPerVector * (kMostSheets - m_sheets.size()));
    put(rulings);

    updates.updateDynamicBuffer(m_backgroundUniforms.get(), 0,
                                static_cast<quint32>(std::span{uniforms}.size_bytes()),
                                uniforms.data());
}

// Ink is drawn wherever it was written, on the sheet or beside it, so only the window cuts it off.
QRhiScissor QtInkRenderer::inkScissor(const QSize& outputSize) {
    return QRhiScissor{0, 0, outputSize.width(), outputSize.height()};
}

void QtInkRenderer::render(QRhiCommandBuffer* commandBuffer) {
    QRhiResourceUpdateBatch* updates = rhi()->nextResourceUpdateBatch();
    uploadStream(m_ink, *updates);
    uploadStream(m_highlights, *updates);
    uploadStream(m_overlay, *updates);
    updateLayerTarget();
    updateBackground(*updates);
    updateMedia(*updates);

    QMatrix4x4 projection = rhi()->clipSpaceCorrMatrix();
    projection.ortho(0.0F, m_logicalWidth, m_logicalHeight, 0.0F, -1.0F, 1.0F);
    projection.scale(m_viewport.scale());
    projection.translate(-m_viewport.origin().x, -m_viewport.origin().y);
    updates->updateDynamicBuffer(m_uniformBuffer.get(), 0, kMatrixBytes, projection.constData());

    // The quad that lays the layer over the page is in clip space, so the picture has to be
    // turned over wherever the first row of a texture is its top.
    const std::array<float, kVectorBytes / sizeof(float)> layerFlip{
        rhi()->isYUpInFramebuffer() ? 0.0F : 1.0F,
        0.0F,
        0.0F,
        0.0F,
    };
    updates->updateDynamicBuffer(m_layerUniforms.get(), 0, kVectorBytes, layerFlip.data());

    const bool drawsLayer = !m_highlights.data.empty() && m_layerTarget;
    if (drawsLayer) {
        commandBuffer->beginPass(m_layerTarget.get(), Qt::transparent, {1.0F, 0}, updates);
        const QSize layerSize = m_layerTexture->pixelSize();
        commandBuffer->setViewport(QRhiViewport{0.0F, 0.0F, static_cast<float>(layerSize.width()),
                                                static_cast<float>(layerSize.height())});
        commandBuffer->setGraphicsPipeline(m_layerPipeline.get());
        commandBuffer->setShaderResources(m_bindings.get());
        const QRhiCommandBuffer::VertexInput layerInput{m_highlights.buffer.get(), 0};
        commandBuffer->setVertexInput(0, 1, &layerInput);
        commandBuffer->draw(static_cast<quint32>(m_highlights.data.size()));
        commandBuffer->endPass();
        updates = rhi()->nextResourceUpdateBatch();
    }

    commandBuffer->beginPass(renderTarget(), Qt::white, {1.0F, 0}, updates);
    const QSize outputSize = renderTarget()->pixelSize();
    commandBuffer->setViewport(QRhiViewport{0.0F, 0.0F, static_cast<float>(outputSize.width()),
                                            static_cast<float>(outputSize.height())});

    commandBuffer->setGraphicsPipeline(m_backgroundPipeline.get());
    commandBuffer->setShaderResources();
    const QRhiCommandBuffer::VertexInput backgroundInput{m_backgroundVertices.get(), 0};
    commandBuffer->setVertexInput(0, 1, &backgroundInput);
    commandBuffer->draw(static_cast<quint32>(kBackgroundCorners.size() / 2));

    for (const MediaEntry& entry : m_media) {
        if (entry.picture.isNull() || entry.area.isEmpty() || !entry.bindings) {
            continue;
        }
        commandBuffer->setGraphicsPipeline(m_mediaPipeline.get());
        commandBuffer->setShaderResources(entry.bindings.get());
        const QRhiCommandBuffer::VertexInput mediaInput{m_backgroundVertices.get(), 0};
        commandBuffer->setVertexInput(0, 1, &mediaInput);
        commandBuffer->draw(static_cast<quint32>(kBackgroundCorners.size() / 2));
    }

    if (drawsLayer) {
        commandBuffer->setGraphicsPipeline(m_compositePipeline.get());
        commandBuffer->setScissor(inkScissor(outputSize));
        commandBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput compositeInput{m_backgroundVertices.get(), 0};
        commandBuffer->setVertexInput(0, 1, &compositeInput);
        commandBuffer->draw(static_cast<quint32>(kBackgroundCorners.size() / 2));
    }

    if (!m_ink.data.empty()) {
        commandBuffer->setGraphicsPipeline(m_pipeline.get());
        commandBuffer->setScissor(inkScissor(outputSize));
        commandBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexInput{m_ink.buffer.get(), 0};
        commandBuffer->setVertexInput(0, 1, &vertexInput);
        commandBuffer->draw(static_cast<quint32>(m_ink.data.size()));
    }

    if (!m_overlay.data.empty()) {
        commandBuffer->setGraphicsPipeline(m_pipeline.get());
        commandBuffer->setScissor(inkScissor(outputSize));
        commandBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput overlayInput{m_overlay.buffer.get(), 0};
        commandBuffer->setVertexInput(0, 1, &overlayInput);
        commandBuffer->draw(static_cast<quint32>(m_overlay.data.size()));
    }
    commandBuffer->endPass();
}

}
