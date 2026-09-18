#include "app/cpp/Thumbnails.hpp"

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>

#include <iterator>

namespace phvikapen::app::thumbnails {
namespace {

// The window paints the pictures; the engine asks for them from whichever thread loads an image.
QMutex& guard() {
    static QMutex mutex;
    return mutex;
}

QHash<QString, QImage>& pictures() {
    static QHash<QString, QImage> kept;
    return kept;
}

class Provider final : public QQuickImageProvider {
public:
    Provider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(const QString& id, QSize* size, const QSize& wanted) override {
        QImage picture = lookUp(id);
        if (picture.isNull()) {
            return picture;
        }
        if (wanted.isValid() && !wanted.isEmpty() && wanted != picture.size()) {
            picture = picture.scaled(wanted, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if (size != nullptr) {
            *size = picture.size();
        }
        return picture;
    }
};

}

void put(const QString& key, const QImage& picture) {
    const QMutexLocker locked{&guard()};
    pictures().insert(key, picture);
}

QImage lookUp(const QString& key) {
    const QMutexLocker locked{&guard()};
    return pictures().value(key);
}

void forget(const QString& pageId) {
    const QMutexLocker locked{&guard()};
    for (auto it = pictures().begin(); it != pictures().end();) {
        it = it.key().startsWith(pageId) ? pictures().erase(it) : std::next(it);
    }
}

QQmlImageProviderBase* makeProvider() {
    return new Provider;
}

}
