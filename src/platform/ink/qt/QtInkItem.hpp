#pragma once

#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "platform/ink/IInkBackend.hpp"

#include <QColor>
#include <QPointer>
#include <QQuickRhiItem>
#include <QQuickWindow>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

class QTabletEvent;

namespace phvikapen::platform::ink {

/// Qt fallback ink backend: a Qt Quick item that captures pen and mouse input and renders wet ink
/// incrementally through QRhi (Metal on macOS, Direct3D on Windows).
///
/// Qt Quick items have no tablet event handler, so tablet events are observed on the window and
/// accepted when they start inside the item; accepted tablet events are not turned into mouse
/// events. Mouse input is supported for development. The item lives on the GUI thread and calls
/// the sink there.
class QtInkItem : public QQuickRhiItem, public IInkBackend {
    Q_OBJECT
    Q_PROPERTY(
        QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(
        qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeStyleChanged FINAL)

public:
    explicit QtInkItem(QQuickItem* parent = nullptr);

    [[nodiscard]] QColor strokeColor() const;
    void setStrokeColor(const QColor& color);

    [[nodiscard]] qreal strokeWidth() const;
    void setStrokeWidth(qreal width);

    /// Removes all strokes, including the one being written.
    Q_INVOKABLE void clear();

    [[nodiscard]] std::string_view name() const noexcept override;
    void setSink(IInkSink* sink) noexcept override;
    void setStrokeStyle(const core::StrokeStyle& style) override;

    /// Tessellated vertices of all strokes, read by the renderer while the GUI thread is blocked.
    [[nodiscard]] const std::vector<core::InkVertex>& vertices() const noexcept {
        return m_vertices;
    }

    /// Changes whenever vertices were removed, so the renderer must upload them again.
    [[nodiscard]] std::uint64_t generation() const noexcept { return m_generation; }

signals:
    void strokeStyleChanged();

protected:
    [[nodiscard]] QQuickRhiItemRenderer* createRenderer() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseUngrabEvent() override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void observeWindow(QQuickWindow* window);
    [[nodiscard]] bool handleTabletEvent(QTabletEvent& event);

    void beginStroke(const core::InkSample& sample);
    void appendToStroke(const core::InkSample& sample);
    void endStroke(const core::InkSample& sample);
    void cancelStroke();

    core::Uuid7Generator m_ids;
    core::StrokeStyle m_style;
    IInkSink* m_sink{nullptr};
    std::optional<core::Stroke> m_activeStroke;
    std::size_t m_activeStrokeFirstVertex{0};
    // TODO(M2): Hand finished strokes over to the document model instead of keeping them here.
    std::vector<core::Stroke> m_strokes;
    std::vector<core::InkVertex> m_vertices;
    std::uint64_t m_generation{0};
    QPointer<QQuickWindow> m_observedWindow;
};

} // namespace phvikapen::platform::ink
