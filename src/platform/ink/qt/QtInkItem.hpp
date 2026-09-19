#pragma once

#include "core/filter/InkFilter.hpp"
#include "core/geometry/Viewport.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "core/ink/StrokeShapes.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "platform/ink/IInkBackend.hpp"

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QPointer>
#include <QQuickRhiItem>
#include <QQuickWindow>
#include <QRectF>
#include <QSize>

#include <chrono>
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
    Q_PROPERTY(bool selecting READ selecting WRITE setSelecting NOTIFY selectingChanged FINAL)
    Q_PROPERTY(bool panning READ panning WRITE setPanning NOTIFY panningChanged FINAL)
    Q_PROPERTY(bool picking READ picking WRITE setPicking NOTIFY pickingChanged FINAL)
    Q_PROPERTY(int shape READ shape WRITE setShape NOTIFY shapeChanged FINAL)
    Q_PROPERTY(qreal corner READ corner WRITE setCorner NOTIFY shapeChanged FINAL)
    Q_PROPERTY(qreal smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged FINAL)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QRectF selectionRect READ selectionRect NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QRectF mediaArea READ mediaArea NOTIFY mediaChanged FINAL)
    Q_PROPERTY(QSize mediaSize READ mediaSize NOTIFY mediaChanged FINAL)
    Q_PROPERTY(int visibleSheetCount READ visibleSheetCount NOTIFY viewChanged FINAL)
    Q_PROPERTY(QColor deskColor READ deskColor WRITE setDeskColor NOTIFY deskColorChanged FINAL)
    Q_PROPERTY(qreal zoom READ zoom NOTIFY viewChanged FINAL)
    Q_PROPERTY(QPointF viewOrigin READ viewOrigin NOTIFY viewChanged FINAL)
    Q_PROPERTY(QPointF pointerAt READ pointerAt NOTIFY pointerChanged FINAL)
    Q_PROPERTY(bool pointerInside READ pointerInside NOTIFY pointerChanged FINAL)

public:
    static constexpr qreal kDefaultEraserRadius = 8.0;
    // The light grey the window had before a theme said otherwise.
    static constexpr QRgb kDefaultDesk = 0xFFE3E6E8;

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

    // One page of a column, with the paper it is written on.
    struct PageView {
        const core::Page* page{nullptr};
        core::PageStyle style;
    };

    // A sheet as the renderer draws it, in the coordinates of the column.
    struct VisibleSheet {
        core::Rect area;
        core::PageStyle style;
    };

    // A picture that belongs to a page, in that page's own coordinates.
    struct MediaPiece {
        core::Uuid page;
        QImage picture;
        QRectF area;
    };

    struct MediaDraw {
        QImage picture;
        QRectF area;
    };

    void showPage(const core::Page& page, const core::PageStyle& style,
                  std::span<const core::Uuid> hidden = {},
                  std::span<const core::Stroke> extra = {});

    void showColumn(std::span<const PageView> pages, int current,
                    std::span<const core::Uuid> hidden = {},
                    std::span<const core::Stroke> extra = {});

    [[nodiscard]] std::vector<VisibleSheet> visibleSheets() const;

    [[nodiscard]] int visibleSheetCount() const { return static_cast<int>(visibleSheets().size()); }

    [[nodiscard]] const std::vector<MediaDraw>& mediaDraws() const noexcept { return m_mediaDraws; }

    Q_INVOKABLE void goToSheet(int index);

    // Where a sheet of the column stands, in the coordinates of the column.
    Q_INVOKABLE [[nodiscard]] QRectF sheetRect(int index) const;

    void showView(const core::Viewport& viewport);

    void showSelection(std::vector<core::Uuid> strokeIds);

    void forgetStrokes(std::span<const core::Uuid> strokeIds);

    static constexpr qreal kDefaultSmoothing = 0.5;

    [[nodiscard]] qreal smoothing() const noexcept { return m_smoothing; }

    void setSmoothing(qreal smoothing);

    [[nodiscard]] int shape() const noexcept { return static_cast<int>(m_shape); }

    void setShape(int shape);

    [[nodiscard]] qreal corner() const noexcept { return m_corner; }

    void setCorner(qreal corner);

    [[nodiscard]] bool panning() const noexcept { return m_panning; }

    void setPanning(bool panning);

    [[nodiscard]] bool selecting() const noexcept { return m_selecting; }

    void setSelecting(bool selecting) override;

    [[nodiscard]] QColor deskColor() const { return m_deskColor; }

    void setDeskColor(const QColor& colour);

    [[nodiscard]] bool picking() const noexcept { return m_picking; }

    void setPicking(bool picking) override;

    [[nodiscard]] int selectedCount() const noexcept { return static_cast<int>(m_selected.size()); }

    [[nodiscard]] const std::vector<core::Uuid>& selection() const noexcept { return m_selected; }

    [[nodiscard]] QRectF selectionRect() const;

    Q_INVOKABLE void clearSelection();

    void showMedia(std::span<const MediaPiece> pieces);
    void clearMedia();

    [[nodiscard]] QImage media() const;

    [[nodiscard]] QRectF mediaArea() const;

    // How many pixels the document of the page being read was drawn with.
    [[nodiscard]] QSize mediaSize() const { return media().size(); }

    [[nodiscard]] std::uint64_t mediaGeneration() const noexcept { return m_mediaGeneration; }

    [[nodiscard]] qreal zoom() const noexcept { return m_viewport.scale(); }

    [[nodiscard]] QPointF viewOrigin() const noexcept;

    [[nodiscard]] QPointF pointerAt() const noexcept { return m_pointerAt; }

    [[nodiscard]] bool pointerInside() const noexcept { return m_pointerInside; }

    [[nodiscard]] const core::Viewport& viewport() const noexcept { return m_viewport; }

    [[nodiscard]] core::Rect visiblePage() const noexcept;

    // The part of the current page that is in the window, in that page's own coordinates.
    [[nodiscard]] core::Rect visibleOnPage() const noexcept;

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

    [[nodiscard]] const std::vector<core::InkVertex>& overlay() const noexcept { return m_overlay; }

    [[nodiscard]] const std::vector<core::InkVertex>& highlights() const noexcept {
        return m_highlights;
    }

    [[nodiscard]] const std::vector<core::InkVertex>& vertices() const noexcept {
        return m_vertices;
    }

    [[nodiscard]] std::uint64_t generation() const noexcept { return m_generation; }

signals:
    void strokeStyleChanged();
    void erasingChanged();
    void selectingChanged();
    void panningChanged();
    void pickingChanged();
    void deskColorChanged();
    void shapeChanged();
    void smoothingChanged();
    void selectionChanged();
    void mediaChanged();
    void pageWanted(int index);
    void eraserRadiusChanged();
    void pointerChanged();
    void pressureSensitiveChanged();
    void viewChanged();

protected:
    [[nodiscard]] QQuickRhiItemRenderer* createRenderer() override;

    void hoverEnterEvent(QHoverEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;
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
    void showPointerAt(const QPointF& position, bool inside);
    [[nodiscard]] bool handleTabletEvent(QTabletEvent& event);
    [[nodiscard]] bool handleNativeGesture(const QNativeGestureEvent& event);
    [[nodiscard]] core::InkSample onPage(core::InkSample sample) const noexcept;
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
    std::size_t m_liveSamples{0};
    std::chrono::steady_clock::time_point m_liveDrawnAt;

    struct StrokeMesh {
        core::Uuid id;
        std::vector<core::InkVertex> vertices;
        float top{};
        bool translucent{};
    };

    struct Sheet {
        core::Uuid id;
        core::PageStyle style;
        float top{};
        float width{};
        float height{};

        friend bool operator==(const Sheet&, const Sheet&) = default;
    };

    [[nodiscard]] std::vector<core::InkVertex>& activeVertices() noexcept;

    void rebuildBuffers();
    void redrawActiveStroke();
    void refreshLiveStroke(bool full);
    void noteKeys(Qt::KeyboardModifiers modifiers);
    void beginMarquee(const core::InkSample& sample);
    void growMarquee(const core::InkSample& sample);
    void finishMarquee();
    void beginDrag(const core::InkSample& sample);
    void dragTo(const core::InkSample& sample);
    void finishDrag();
    [[nodiscard]] bool overSelection(const core::InkSample& sample) const noexcept;
    [[nodiscard]] std::optional<core::Rect> selectionBounds() const noexcept;
    [[nodiscard]] float currentTop() const noexcept;
    // The sheet a gesture belongs to: the one it started on, whatever the reader scrolls to.
    [[nodiscard]] float workingTop() const noexcept;
    [[nodiscard]] int sheetAt(float y) const noexcept;
    [[nodiscard]] std::optional<core::PaperSize> columnSize() const noexcept;
    [[nodiscard]] core::InkSample onSheet(core::InkSample sample) const noexcept;
    [[nodiscard]] core::Point onSheet(core::Point point) const noexcept;
    [[nodiscard]] core::Stroke onSheet(const core::Stroke& stroke) const;
    void rebuildMedia();
    [[nodiscard]] float roomFor(const core::Uuid& page, const core::PaperSize& fallback) const;
    [[nodiscard]] std::vector<Sheet> sheetsFor(std::span<const PageView> pages) const;
    void rebuildMeshes(std::span<const PageView> pages);
    void followScrolling();

    std::vector<Sheet> m_sheets;
    std::vector<MediaPiece> m_media;
    std::vector<MediaDraw> m_mediaDraws;
    int m_current{0};
    int m_workSheet{-1};
    bool m_followingScroll{false};
    std::vector<StrokeMesh> m_meshes;
    std::vector<core::InkVertex> m_vertices;
    std::vector<core::InkVertex> m_highlights;
    std::vector<core::InkVertex> m_overlay;
    std::vector<core::Uuid> m_selected;
    std::vector<core::Uuid> m_hidden;
    std::vector<core::Stroke> m_extra;

    struct Marquee {
        core::Point from;
        core::Point to;
    };

    std::optional<Marquee> m_marquee;
    std::optional<core::Point> m_dragFrom;
    core::Point m_dragOffset;
    core::Shape m_shape{core::Shape::Freehand};
    qreal m_corner{0.0};
    qreal m_smoothing{kDefaultSmoothing};
    core::ShapeKeys m_shapeKeys;
    std::optional<core::Point> m_panFrom;
    bool m_selecting{false};
    bool m_panning{false};
    bool m_picking{false};
    bool m_activeIsTranslucent{false};
    std::uint64_t m_generation{0};
    core::Viewport m_viewport;
    core::PageStyle m_pageStyle;
    QColor m_deskColor{kDefaultDesk};
    std::uint64_t m_mediaGeneration{0};
    core::Uuid m_shownPage;
    bool m_viewFitted{false};
    QPointF m_pointerAt;
    bool m_pointerInside{false};
    std::optional<QPointF> m_touchCentroid;
    qreal m_touchSpread{0.0};
    QPointer<QQuickWindow> m_observedWindow;
};

}
