#pragma once

#include <QImage>
#include <QString>

class QQmlImageProviderBase;

namespace phvikapen::app::thumbnails {

inline constexpr int kWidth = 96;

void put(const QString& key, const QImage& picture);

[[nodiscard]] QImage lookUp(const QString& key);

void forget(const QString& pageId);

[[nodiscard]] QQmlImageProviderBase* makeProvider();

}
