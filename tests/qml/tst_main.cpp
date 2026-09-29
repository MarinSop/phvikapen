#include "app/cpp/Thumbnails.hpp"

#include <QColor>
#include <QCoreApplication>
#include <QImage>
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

#include <cstdio>

class Setup : public QObject {
    Q_OBJECT

public slots:

    static void applicationAvailable() {
        // Written to a pipe, what a test says is held back until there is enough of it, and a test
        // run that ends badly takes the whole lot with it. Saying it as it happens costs nothing
        // and is the difference between a report and a silence.
        std::setvbuf(stdout, nullptr, _IONBF, 0);
        std::setvbuf(stderr, nullptr, _IONBF, 0);
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
        engine->rootContext()->setContextProperty(QStringLiteral("samplePicture"),
                                                  writeSamplePicture());
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

    [[nodiscard]] QString writeSamplePicture() const {
        const QString path = m_directory.filePath(QStringLiteral("sample.png"));
        QImage picture{40, 20, QImage::Format_ARGB32};
        picture.fill(QColor{Qt::red});
        picture.save(path);
        return path;
    }

    QTemporaryDir m_directory;
};

QUICK_TEST_MAIN_WITH_SETUP(phvikapen_qml, Setup)

#include "tst_main.moc"
