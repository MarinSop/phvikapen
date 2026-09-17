#pragma once

#include "core/filter/InkFilter.hpp"
#include "core/geometry/Viewport.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "platform/ink/IInkBackend.hpp"

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QPointer>
#include <QQuickRhiItem>
#include <QQuickWindow>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

class QNativeGestureEvent;
class QTabletEvent;
class QTouchEvent;
class QWheelEvent;

namespace phvikapen::platform::ink {

class QtInkItem : public QQuickRhiItem, public IInkBackend {
    Q_OBJECT
    Q_PROPERTY(
        QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(
        qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(bool erasing READ erasing WRITE setErasing NOTIFY erasingChanged FINAL)
    Q_PROPERTY(
        qreal eraserRadius READ eraserRadius WRITE setEraserRadius NOTIFY eraserRadiusChanged FINAL)
    Q_PROPERTY(bool pressureSensitive READ pressureSensitive WRITE setPressureSensitive NOTIFY
                   pressureSensitiveChanged FINAL)
    Q_PROPERTY(qreal zoom READ zoom NOTIFY viewChanged FINAL)
    Q_PROPERTY(QPointF viewOrigin READ viewOrigin NOTIFY viewChanged FINAL)

public:
    static constexpr qreal kDefaultEraserRadius = 8.0;

    explicit QtInkItem(QQuickItem* parent = nullptr);
    ~QtInkItem() override;

    QtInkItem(const QtInkItem&) = delete;
    QtInkItem& operator=(const QtInkItem&) = delete;
    QtInkItem(QtInkItem&&) = delete;
    QtInkItem& operator=(QtInkItem&&) = delete;

    [[nodiscard]] QColor strokeColor() const;
    void setStrokeColor(const QColor& color);

    [[nodiscard]] qreal strokeWidth() const;
    void setStrokeWidth(qreal width);

    Q_INVOKABLE void clear();

    void showPage(const core::Page& page, const core::PageStyle& style,
                  std::span<const core::Uuid> hidden = {});

    void showMedia(const QImage& image);
    void clearMedia();

    [[nodiscard]] const QImage& media() const noexcept { return m_media; }

    [[nodiscard]] std::uint64_t mediaGeneration() const noexcept { return m_mediaGeneration; }

    [[nodiscard]] qreal zoom() const noexcept { return m_viewport.scale(); }

    [[nodiscard]] QPointF viewOrigin() const noexcept;

    [[nodiscard]] const core::Viewport& viewport() const noexcept { return m_viewport; }

    [[nodiscard]] const core::PageStyle& pageStyle() const noexcept { return m_pageStyle; }

    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void fitPage();

    [[nodiscard]] std::string_view name() const noexcept override;
    void setSink(IInkSink* sink) noexcept override;
    void setStrokeStyle(const core::StrokeStyle& style) override;

    [[nodiscard]] bool erasing() const noexcept { return m_erasing; }

    void setErasing(bool erasing) override;

    [[nodiscard]] qreal eraserRadius() const noexcept { return m_eraserRadius; }

    void setEraserRadius(qreal radius);

    [[nodiscard]] bool pressureSensitive() const noexcept { return m_pressureSensitive; }

    void setPressureSensitive(bool sensitive);

    [[nodiscard]] const std::vector<core::InkVertex>& vertices() const noexcept {
        return m_vertices;
    }

    [[nodiscard]] std::uint64_t generation() const noexcept { return m_generation; }

signals:
    void strokeStyleChanged();
    void erasingChanged();
    void eraserRadiusChanged();
    void pressureSensitiveChanged();
    void viewChanged();

protected:
    [[nodiscard]] QQuickRhiItemRenderer* createRenderer() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseUngrabEvent() override;
    void wheelEvent(QWheelEvent* event) override;
    void touchEvent(QTouchEvent* event) override;
    void touchUngrabEvent() override;
    bool event(QEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void observeWindow(QQuickWindow* window);
    [[nodiscard]] bool handleTabletEvent(QTabletEvent& event);
    [[nodiscard]] bool handleNativeGesture(const QNativeGestureEvent& event);
    [[nodiscard]] core::InkSample onPage(core::InkSample sample) const noexcept;
    [[nodiscard]] bool onPaper(const core::InkSample& sample) const noexcept;
    [[nodiscard]] core::ViewSize viewSize() const noexcept;
    void changeView(const core::Viewport& viewport);

    void press(const core::InkSample& sample, bool eraserTip);
    void move(const core::InkSample& sample);
    void release(const core::InkSample& sample);
    [[nodiscard]] bool isTracking() const noexcept;

    void beginErase(const core::InkSample& sample);
    void moveEraser(const core::InkSample& sample);
    void finishErase();

    void beginStroke(const core::InkSample& sample);
    void appendToStroke(const core::InkSample& sample);
    void endStroke(const core::InkSample& sample);
    void cancelStroke();

    core::Uuid7Generator m_ids;
    core::InkFilter m_filter;
    core::StrokeStyle m_style;
    IInkSink* m_sink{nullptr};
    std::optional<core::Stroke> m_activeStroke;
    std::optional<core::InkSample> m_eraserPosition;
    bool m_erasing{false};
    bool m_pressureSensitive{true};
    qreal m_eraserRadius{kDefaultEraserRadius};
    std::size_t m_activeStrokeFirstVertex{0};
    std::vector<core::InkVertex> m_vertices;
    std::uint64_t m_generation{0};
    core::Viewport m_viewport;
    core::PageStyle m_pageStyle;
    QImage m_media;
    std::uint64_t m_mediaGeneration{0};
    core::Uuid m_shownPage;
    bool m_viewFitted{false};
    std::optional<QPointF> m_touchCentroid;
    qreal m_touchSpread{0.0};
    QPointer<QQuickWindow> m_observedWindow;
};

}
