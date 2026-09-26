#include "app/cpp/Clipboard.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QString>

namespace phvikapen::app {

Clipboard::Clipboard(QObject* parent) : QObject(parent) {}

void Clipboard::put(const QString& text) {
    if (QClipboard* const board = QGuiApplication::clipboard(); board != nullptr) {
        board->setText(text);
    }
}

QString Clipboard::taken() {
    const QClipboard* const board = QGuiApplication::clipboard();
    return board == nullptr ? QString{} : board->text();
}

bool Clipboard::holdsText() {
    return !taken().isEmpty();
}

}
