#pragma once

#include <QFile>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include <QTextStream>
#include <QtQmlIntegration>

class QPointerEvent;
class QEventPoint;
class QTabletEvent;

namespace phvikapen::tools {

/// Captures raw pen and mouse input and appends one CSV row per event.
///
/// Tablet events are observed on the window, because Qt Quick items have no tablet event handler.
/// Unlike the canvas, the recorder does not accept them: the samples are only written down, and
/// nothing is drawn. A hovering pen is recorded as well, as a tablet event without pressure.
/// Mouse hovering is not recorded: Qt Quick pairs every hover event with a synthesized one that
/// carries no timestamp, and a hovering mouse says nothing about how a pen behaves.
class RecorderItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged FINAL)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged FINAL)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY sampleCountChanged FINAL)

public:
    explicit RecorderItem(QQuickItem* parent = nullptr);
    ~RecorderItem() override;

    RecorderItem(const RecorderItem&) = delete;
    RecorderItem& operator=(const RecorderItem&) = delete;
    RecorderItem(RecorderItem&&) = delete;
    RecorderItem& operator=(RecorderItem&&) = delete;

    [[nodiscard]] bool isRecording() const { return m_file.isOpen(); }

    [[nodiscard]] QString filePath() const { return m_filePath; }

    [[nodiscard]] int sampleCount() const { return m_sampleCount; }

    /// Opens a new recording file below QStandardPaths::AppLocalDataLocation.
    Q_INVOKABLE void start();

    /// Closes the current recording file.
    Q_INVOKABLE void stop();

signals:
    void recordingChanged();
    void filePathChanged();
    void sampleCountChanged();
    void failed(const QString& message);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void observeWindow(QQuickWindow* window);
    void writeEvent(const QPointerEvent& event, const QEventPoint& point, QStringView kind);

    QString m_filePath;
    QFile m_file;
    QTextStream m_stream;
    int m_sampleCount{0};
    QPointer<QQuickWindow> m_observedWindow;
};

} // namespace phvikapen::tools
