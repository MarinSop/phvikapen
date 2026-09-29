#include "app/cpp/Thumbnails.hpp"

#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QImage>
#include <QObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
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
        // The tests must meet the controls the application is built out of. Left to itself, a
        // machine picks its own style, and on Windows that one refuses to be dressed at all.
        QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));
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
        engine->rootContext()->setContextProperty(QStringLiteral("paper"), this);
    }

public:
    // Words written onto a sheet with a brush rather than grabbed from what is on screen: a machine
    // with no screen behind it hands back what it drew on some platforms and not on others, and a
    // reader given a blank sheet cannot be told apart from a reader that reads nothing.
    Q_INVOKABLE static bool writeWords(const QString& path, const QString& words) {
        constexpr int kWide = 480;
        constexpr int kHigh = 200;
        constexpr int kLetters = 56;
        QImage sheet{kWide, kHigh, QImage::Format_RGB32};
        sheet.fill(Qt::white);
        QPainter painter{&sheet};
        QFont face = painter.font();
        face.setPixelSize(kLetters);
        painter.setFont(face);
        painter.setPen(QColor{Qt::black});
        painter.drawText(sheet.rect(), Qt::AlignCenter, words);
        painter.end();
        return anyLetteringIn(sheet) && sheet.save(path);
    }

private:
    [[nodiscard]] static bool anyLetteringIn(const QImage& sheet) {
        constexpr int kDark = 100;
        constexpr int kEnough = 40;
        int dark = 0;
        for (int y = 0; y < sheet.height(); ++y) {
            for (int x = 0; x < sheet.width(); ++x) {
                if (qGray(sheet.pixel(x, y)) < kDark) {
                    ++dark;
                    if (dark >= kEnough) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

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
