#pragma once

#include "core/geometry/Viewport.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "core/model/PageStyle.hpp"

#include <QQuickRhiItem>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiResourceUpdateBatch;
class QRhiShaderResourceBindings;
class QShader;

namespace phvikapen::platform::ink {

class QtInkRenderer final : public QQuickRhiItemRenderer {
public:
    QtInkRenderer();
    ~QtInkRenderer() override;

    QtInkRenderer(const QtInkRenderer&) = delete;
    QtInkRenderer& operator=(const QtInkRenderer&) = delete;
    QtInkRenderer(QtInkRenderer&&) = delete;
    QtInkRenderer& operator=(QtInkRenderer&&) = delete;

    void initialize(QRhiCommandBuffer* commandBuffer) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* commandBuffer) override;

private:
    void createInkPipeline();
    void createBackgroundPipeline();
    void uploadVertices(QRhiResourceUpdateBatch& updates);
    void updateBackground(QRhiResourceUpdateBatch& updates);

    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::unique_ptr<QRhiBuffer> m_backgroundVertices;
    std::unique_ptr<QRhiBuffer> m_backgroundUniforms;
    std::unique_ptr<QRhiShaderResourceBindings> m_backgroundBindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_backgroundPipeline;
    bool m_backgroundVerticesUploaded{false};
    int m_sampleCount{0};

    std::vector<core::InkVertex> m_vertices;
    std::size_t m_uploadedVertexCount{0};
    std::uint64_t m_generation{0};
    float m_logicalWidth{0.0F};
    float m_logicalHeight{0.0F};
    core::Viewport m_viewport;
    core::PageStyle m_pageStyle;
};

}
