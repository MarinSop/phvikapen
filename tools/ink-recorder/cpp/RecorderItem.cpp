#include "RecorderItem.hpp"

#include <QDateTime>
#include <QDir>
#include <QEventPoint>
#include <QMouseEvent>
#include <QPointerEvent>
#include <QPointingDevice>
#include <QStandardPaths>
#include <QTabletEvent>

namespace phvikapen::tools {
namespace {

constexpr int kMicrosecondsPerMillisecond = 1000;
constexpr int kCoordinateDecimals = 3;
constexpr int kPressureDecimals = 4;

[[nodiscard]] QString pointerTypeName(QPointingDevice::PointerType type) {
    switch (type) {
    case QPointingDevice::PointerType::Generic:
        return QStringLiteral("mouse");
    case QPointingDevice::PointerType::Finger:
        return QStringLiteral("finger");
    case QPointingDevice::PointerType::Pen:
        return QStringLiteral("pen");
    case QPointingDevice::PointerType::Eraser:
        return QStringLiteral("eraser");
    case QPointingDevice::PointerType::Cursor:
        return QStringLiteral("cursor");
    case QPointingDevice::PointerType::AllPointerTypes:
    case QPointingDevice::PointerType::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

}

RecorderItem::RecorderItem(QQuickItem* parent) : QQuickItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    connect(this, &QQuickItem::windowChanged, this, &RecorderItem::observeWindow);
}

RecorderItem::~RecorderItem() {
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

void RecorderItem::start() {
    if (m_file.isOpen()) {
        return;
    }

    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recordings";
    if (!QDir().mkpath(directory)) {
        emit failed(tr("Could not create %1").arg(directory));
        return;
    }

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    m_filePath = directory + "/recording-" + stamp + ".csv";
    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit failed(tr("Could not open %1: %2").arg(m_filePath, m_file.errorString()));
        return;
    }

    m_stream.setDevice(&m_file);
    m_stream << "timestamp_us,event,x,y,pressure,tilt_x,tilt_y,pointer_type,device_id\n";
    m_stream.flush();

    m_sampleCount = 0;
    emit sampleCountChanged();
    emit filePathChanged();
    emit recordingChanged();
}

void RecorderItem::stop() {
    if (!m_file.isOpen()) {
        return;
    }
    m_stream.flush();
    m_stream.setDevice(nullptr);
    m_file.close();
    emit recordingChanged();
}

void RecorderItem::writeEvent(const QPointerEvent& event, const QEventPoint& point,
                              QStringView kind) {
    if (!m_file.isOpen()) {
        return;
    }

    const QPointingDevice* const device = event.pointingDevice();
    const QPointF position = mapFromScene(point.scenePosition());
    const auto timestampUs = static_cast<qint64>(event.timestamp()) * kMicrosecondsPerMillisecond;

    qreal tiltX = 0.0;
    qreal tiltY = 0.0;
    if (const auto* const tablet = dynamic_cast<const QTabletEvent*>(&event)) {
        tiltX = tablet->xTilt();
        tiltY = tablet->yTilt();
    }

    m_stream << timestampUs << ',' << kind << ','
             << QString::number(position.x(), 'f', kCoordinateDecimals) << ','
             << QString::number(position.y(), 'f', kCoordinateDecimals) << ','
             << QString::number(point.pressure(), 'f', kPressureDecimals) << ','
             << QString::number(tiltX, 'f', kCoordinateDecimals) << ','
             << QString::number(tiltY, 'f', kCoordinateDecimals) << ','
             << pointerTypeName(device != nullptr ? device->pointerType()
                                                  : QPointingDevice::PointerType::Unknown)
             << ',' << (device != nullptr ? device->uniqueId().numericId() : -1) << '\n';

    ++m_sampleCount;
    emit sampleCountChanged();
}

void RecorderItem::mousePressEvent(QMouseEvent* event) {
    writeEvent(*event, event->points().constFirst(), u"press");
    event->accept();
}

void RecorderItem::mouseMoveEvent(QMouseEvent* event) {
    writeEvent(*event, event->points().constFirst(), u"move");
    event->accept();
}

void RecorderItem::mouseReleaseEvent(QMouseEvent* event) {
    writeEvent(*event, event->points().constFirst(), u"release");
    event->accept();
}

bool RecorderItem::eventFilter(QObject* watched, QEvent* event) {
    auto* const tabletEvent = dynamic_cast<QTabletEvent*>(event);
    if (tabletEvent == nullptr || watched != m_observedWindow) {
        return QQuickItem::eventFilter(watched, event);
    }

    const QPointF local = mapFromScene(tabletEvent->scenePosition());
    if (!isVisible() || !contains(local)) {
        return false;
    }

    switch (tabletEvent->type()) {
    case QEvent::TabletPress:
        writeEvent(*tabletEvent, tabletEvent->points().constFirst(), u"press");
        break;
    case QEvent::TabletMove:
        writeEvent(*tabletEvent, tabletEvent->points().constFirst(),
                   tabletEvent->pressure() > 0.0 ? u"move" : u"hover");
        break;
    case QEvent::TabletRelease:
        writeEvent(*tabletEvent, tabletEvent->points().constFirst(), u"release");
        break;
    default:
        break;
    }

    return false;
}

void RecorderItem::observeWindow(QQuickWindow* window) {
    if (m_observedWindow) {
        m_observedWindow->removeEventFilter(this);
    }
    m_observedWindow = window;
    if (m_observedWindow) {
        m_observedWindow->installEventFilter(this);
    }
}

}
