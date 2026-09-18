#include "app/cpp/Thumbnails.hpp"

#include <QColor>
#include <QCoreApplication>
#include <QObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRect>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QtQuickTest/quicktest.h>

class Setup : public QObject {
    Q_OBJECT

public slots:

    static void applicationAvailable() {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("PhvikaPenTests"));
        QCoreApplication::setApplicationName(QStringLiteral("PhvikaPenTests"));
        QSettings settings;
        settings.clear();
    }

    void qmlEngineAvailable(QQmlEngine* engine) {
        engine->addImageProvider(QStringLiteral("pages"),
                                 phvikapen::app::thumbnails::makeProvider());
        engine->rootContext()->setContextProperty(QStringLiteral("temporaryDirectory"),
                                                  m_directory.path());
        engine->rootContext()->setContextProperty(QStringLiteral("samplePdf"), writeSamplePdf());
    }

private:
    [[nodiscard]] QString writeSamplePdf() const {
        const QString path = m_directory.filePath(QStringLiteral("sample.pdf"));
        QPdfWriter writer{path};
        writer.setPageSize(QPageSize{QPageSize::A5});
        QPainter painter{&writer};
        painter.fillRect(QRect{0, 0, writer.width() / 2, writer.height() / 2}, QColor{Qt::black});
        writer.newPage();
        painter.fillRect(QRect{0, 0, writer.width(), writer.height() / 4}, QColor{Qt::black});
        painter.end();
        return path;
    }

    QTemporaryDir m_directory;
};

QUICK_TEST_MAIN_WITH_SETUP(phvikapen_qml, Setup)

#include "tst_main.moc"
