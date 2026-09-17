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

    Q_INVOKABLE void start();

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

}
