#pragma once

#include <QColor>
#include <QObject>
#include <QQmlParserStatus>
#include <QVariantList>
#include <QtQmlIntegration>
#include <QtTypes>

#include <array>
#include <cstddef>

namespace phvikapen::app {

class ToolViewModel : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(
        Tool currentTool READ currentTool WRITE setCurrentTool NOTIFY currentToolChanged FINAL)
    Q_PROPERTY(int pen READ pen WRITE setPen NOTIFY penChanged FINAL)
    Q_PROPERTY(Shape shape READ shape WRITE setShape NOTIFY shapeChanged FINAL)
    Q_PROPERTY(int penCount READ penCount CONSTANT FINAL)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY toolChanged FINAL)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY toolChanged FINAL)
    Q_PROPERTY(bool pressureSensitive READ pressureSensitive NOTIFY toolChanged FINAL)
    Q_PROPERTY(
        qreal eraserRadius READ eraserRadius WRITE setEraserRadius NOTIFY eraserChanged FINAL)
    Q_PROPERTY(QVariantList palette READ palette CONSTANT FINAL)

public:
    enum class Shape : quint8 {
        Freehand,
        Line,
        Rectangle,
        Ellipse,
    };
    Q_ENUM(Shape)

    enum class Tool : quint8 {
        Pen,
        Highlighter,
        Eraser,
        Selection,
        Hand,
    };
    Q_ENUM(Tool)

    static constexpr qreal kMinimumWidth = 0.5;
    static constexpr qreal kMaximumWidth = 24.0;
    static constexpr qreal kDefaultEraser = 8.0;
    static constexpr qreal kMinimumEraser = 4.0;
    static constexpr qreal kMaximumEraser = 40.0;

    explicit ToolViewModel(QObject* parent = nullptr);

    void classBegin() override {}

    void componentComplete() override;

    [[nodiscard]] Shape shape() const { return m_shape; }

    void setShape(Shape shape);

    [[nodiscard]] Tool currentTool() const { return m_currentTool; }

    void setCurrentTool(Tool tool);

    [[nodiscard]] int pen() const { return m_pen; }

    void setPen(int index);

    [[nodiscard]] static int penCount() { return static_cast<int>(kPenCount); }

    [[nodiscard]] QColor strokeColor() const;
    void setStrokeColor(const QColor& color);
    [[nodiscard]] qreal strokeWidth() const;
    void setStrokeWidth(qreal width);
    [[nodiscard]] bool pressureSensitive() const;

    [[nodiscard]] qreal eraserRadius() const { return m_eraserRadius; }

    void setEraserRadius(qreal radius);

    [[nodiscard]] static QVariantList palette();

    Q_INVOKABLE [[nodiscard]] QColor colorOfPen(int index) const;
    Q_INVOKABLE [[nodiscard]] qreal widthOfPen(int index) const;

signals:
    void currentToolChanged();
    void shapeChanged();
    void penChanged();
    void toolChanged();
    void eraserChanged();

private:
    struct Nib {
        QColor color;
        qreal width{};
    };

    static constexpr std::size_t kPenCount = 3;

    [[nodiscard]] Nib& activeNib();
    [[nodiscard]] const Nib& activeNib() const;
    void remember() const;
    void restore();

    std::array<Nib, kPenCount> m_pens;
    Nib m_highlighter;
    Tool m_currentTool{Tool::Pen};
    Shape m_shape{Shape::Freehand};
    qreal m_eraserRadius{kDefaultEraser};
    int m_pen{0};
    bool m_completed{false};
};

}
