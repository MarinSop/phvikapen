#pragma once

#include "core/model/TextBox.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtQmlIntegration>
#include <QtTypes>

#include <vector>

namespace phvikapen::app {

// A text box as the window shows it: where it stands in the column of sheets everything is drawn
// in, and what it says.
struct TextItem {
    QString textId;
    QString pageId;
    QString text;
    QString font;
    QColor color;
    qreal columnX{};
    qreal columnY{};
    qreal width{};
    qreal height{};
    qreal size{};
    qreal lineHeight{};
    int align{};
    int sheet{};
    bool bold{};
    bool italic{};
    bool underline{};
    bool struckOut{};

    friend bool operator==(const TextItem&, const TextItem&) = default;
};

class TextListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kTextIdRole = Qt::UserRole + 1;
    static constexpr int kPageIdRole = Qt::UserRole + 2;
    static constexpr int kTextRole = Qt::UserRole + 3;
    static constexpr int kFontRole = Qt::UserRole + 4;
    static constexpr int kColorRole = Qt::UserRole + 5;
    static constexpr int kColumnXRole = Qt::UserRole + 6;
    static constexpr int kColumnYRole = Qt::UserRole + 7;
    static constexpr int kWidthRole = Qt::UserRole + 8;
    static constexpr int kHeightRole = Qt::UserRole + 9;
    static constexpr int kSizeRole = Qt::UserRole + 10;
    static constexpr int kLineHeightRole = Qt::UserRole + 11;
    static constexpr int kAlignRole = Qt::UserRole + 12;
    static constexpr int kSheetRole = Qt::UserRole + 13;
    static constexpr int kBoldRole = Qt::UserRole + 14;
    static constexpr int kItalicRole = Qt::UserRole + 15;
    static constexpr int kUnderlineRole = Qt::UserRole + 16;
    static constexpr int kStruckOutRole = Qt::UserRole + 17;

    explicit TextListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<TextItem> items);

signals:
    void countChanged();

private:
    std::vector<TextItem> m_items;
};

// What a text box wears, as QML hands it about.
[[nodiscard]] QVariantMap mapOfStyle(const core::TextStyle& style);

[[nodiscard]] core::TextStyle styleOfMap(const QVariantMap& style);

}
