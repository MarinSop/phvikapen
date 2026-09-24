#pragma once

#include "core/ink/StrokeEraser.hpp"
#include "core/model/TextBox.hpp"

#include <QColor>
#include <QObject>
#include <QQmlParserStatus>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
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
    Q_PROPERTY(qreal corner READ corner WRITE setCorner NOTIFY shapeChanged FINAL)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY toolChanged FINAL)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY toolChanged FINAL)
    Q_PROPERTY(bool pressureSensitive READ pressureSensitive NOTIFY toolChanged FINAL)
    Q_PROPERTY(
        qreal eraserRadius READ eraserRadius WRITE setEraserRadius NOTIFY eraserChanged FINAL)
    Q_PROPERTY(Erase eraserMode READ eraserMode WRITE setEraserMode NOTIFY eraserChanged FINAL)
    Q_PROPERTY(QVariantList palette READ palette CONSTANT FINAL)
    Q_PROPERTY(QVariantList penColors READ penColors NOTIFY toolChanged FINAL)
    Q_PROPERTY(QVariantList penWidths READ penWidths NOTIFY toolChanged FINAL)
    Q_PROPERTY(QString textFont READ textFont WRITE setTextFont NOTIFY textChanged FINAL)
    Q_PROPERTY(qreal textSize READ textSize WRITE setTextSize NOTIFY textChanged FINAL)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textChanged FINAL)
    Q_PROPERTY(int textAlign READ textAlign WRITE setTextAlign NOTIFY textChanged FINAL)
    Q_PROPERTY(bool textBold READ textBold WRITE setTextBold NOTIFY textChanged FINAL)
    Q_PROPERTY(bool textItalic READ textItalic WRITE setTextItalic NOTIFY textChanged FINAL)
    Q_PROPERTY(
        bool textUnderline READ textUnderline WRITE setTextUnderline NOTIFY textChanged FINAL)
    Q_PROPERTY(
        bool textStruckOut READ textStruckOut WRITE setTextStruckOut NOTIFY textChanged FINAL)
    Q_PROPERTY(QVariantMap textStyle READ textStyle NOTIFY textChanged FINAL)

public:
    enum class Shape : quint8 {
        Freehand,
        Line,
        Rectangle,
        Ellipse,
    };
    Q_ENUM(Shape)

    // What the eraser takes. Named here rather than as a plain switch, so that another way of
    // rubbing out can be added without every caller having to be found again.
    enum class Erase : quint8 {
        Touched,
        WholeStroke,
    };
    Q_ENUM(Erase)

    enum class Tool : quint8 {
        Pen,
        Highlighter,
        Eraser,
        Selection,
        Hand,
        ColourPicker,
        Shape,
        Text,
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

    [[nodiscard]] qreal corner() const { return m_corner; }

    void setCorner(qreal corner);

    static constexpr qreal kMaximumCorner = 60.0;

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

    [[nodiscard]] Erase eraserMode() const { return m_eraserMode; }

    void setEraserMode(Erase mode);

    [[nodiscard]] static QVariantList palette();

    [[nodiscard]] QVariantList penColors() const;
    [[nodiscard]] QVariantList penWidths() const;

    [[nodiscard]] QString textFont() const;
    void setTextFont(const QString& font);
    [[nodiscard]] qreal textSize() const;
    void setTextSize(qreal size);
    [[nodiscard]] QColor textColor() const;
    void setTextColor(const QColor& color);
    [[nodiscard]] int textAlign() const;
    void setTextAlign(int align);

    [[nodiscard]] bool textBold() const { return m_text.bold; }

    void setTextBold(bool bold);

    [[nodiscard]] bool textItalic() const { return m_text.italic; }

    void setTextItalic(bool italic);

    [[nodiscard]] bool textUnderline() const { return m_text.underline; }

    void setTextUnderline(bool underline);

    [[nodiscard]] bool textStruckOut() const { return m_text.struckOut; }

    void setTextStruckOut(bool struckOut);

    [[nodiscard]] QVariantMap textStyle() const;

    // The face a box that is picked wears becomes the face the bar shows and the next box takes.
    Q_INVOKABLE void useTextStyle(const QVariantMap& style);

    // The colour taken off the page belongs to every pen from then on, and the tool that was in
    // hand before the picker comes back.
    Q_INVOKABLE void usePickedColour(const QColor& colour);

    Q_INVOKABLE [[nodiscard]] QColor colorOfPen(int index) const;
    Q_INVOKABLE [[nodiscard]] qreal widthOfPen(int index) const;

signals:
    void currentToolChanged();
    void shapeChanged();
    void penChanged();
    void toolChanged();
    void textChanged();
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
    Tool m_beforePicking{Tool::Pen};
    core::TextStyle m_text;
    Shape m_shape{Shape::Rectangle};
    qreal m_eraserRadius{kDefaultEraser};
    Erase m_eraserMode{Erase::Touched};
    qreal m_corner{0.0};
    int m_pen{0};
    bool m_completed{false};
};

}
