#pragma once

#include "core/filter/InkFilter.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeMesh.hpp"
#include "core/model/Page.hpp"
#include "platform/ink/IInkBackend.hpp"

#include <QColor>
#include <QPointer>
#include <QQuickRhiItem>
#include <QQuickWindow>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

class QTabletEvent;

namespace phvikapen::platform::ink {

class QtInkItem : public QQuickRhiItem, public IInkBackend {
    Q_OBJECT
    Q_PROPERTY(
        QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(
        qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(bool erasing READ erasing WRITE setErasing NOTIFY erasingChanged FINAL)

public:
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

    void showPage(const core::Page& page, std::span<const core::Uuid> hidden = {});

    [[nodiscard]] std::string_view name() const noexcept override;
    void setSink(IInkSink* sink) noexcept override;
    void setStrokeStyle(const core::StrokeStyle& style) override;

    [[nodiscard]] bool erasing() const noexcept { return m_erasing; }

    void setErasing(bool erasing) override;

    [[nodiscard]] const std::vector<core::InkVertex>& vertices() const noexcept {
        return m_vertices;
    }

    [[nodiscard]] std::uint64_t generation() const noexcept { return m_generation; }

signals:
    void strokeStyleChanged();
    void erasingChanged();

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
    std::size_t m_activeStrokeFirstVertex{0};
    std::vector<core::InkVertex> m_vertices;
    std::uint64_t m_generation{0};
    QPointer<QQuickWindow> m_observedWindow;
};

}
