#pragma once

#include <QColor>
#include <QObject>
#include <QProperty>
#include <QtQmlIntegration>

namespace phvikapen::app {

class ToolViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Tool currentTool READ currentTool WRITE setCurrentTool NOTIFY currentToolChanged
                   BINDABLE bindableCurrentTool FINAL)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged
                   BINDABLE bindableStrokeColor FINAL)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged
                   BINDABLE bindableStrokeWidth FINAL)

public:
    // No std::uint8_t base: the QML type registrar cannot resolve it.
    enum class Tool {
        Pen,
        Eraser,
    };
    Q_ENUM(Tool)

    explicit ToolViewModel(QObject* parent = nullptr) : QObject(parent) {
        m_strokeColor = QColor(Qt::black);
        m_strokeWidth = kDefaultStrokeWidth;
    }

    [[nodiscard]] Tool currentTool() const { return m_currentTool.value(); }

    void setCurrentTool(Tool tool) { m_currentTool = tool; }

    [[nodiscard]] QBindable<Tool> bindableCurrentTool() { return {&m_currentTool}; }

    [[nodiscard]] QColor strokeColor() const { return m_strokeColor.value(); }

    void setStrokeColor(const QColor& color) { m_strokeColor = color; }

    [[nodiscard]] QBindable<QColor> bindableStrokeColor() { return {&m_strokeColor}; }

    [[nodiscard]] qreal strokeWidth() const { return m_strokeWidth.value(); }

    void setStrokeWidth(qreal width) { m_strokeWidth = width; }

    [[nodiscard]] QBindable<qreal> bindableStrokeWidth() { return {&m_strokeWidth}; }

signals:
    void currentToolChanged();
    void strokeColorChanged();
    void strokeWidthChanged();

private:
    static constexpr qreal kDefaultStrokeWidth = 2.0;

    Q_OBJECT_BINDABLE_PROPERTY(ToolViewModel, Tool, m_currentTool,
                               &ToolViewModel::currentToolChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ToolViewModel, QColor, m_strokeColor,
                               &ToolViewModel::strokeColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ToolViewModel, qreal, m_strokeWidth,
                               &ToolViewModel::strokeWidthChanged)
};

}
