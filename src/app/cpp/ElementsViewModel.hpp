#pragma once

#include "app/cpp/NotebookViewModel.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/storage/StorageThread.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QModelIndex>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration>
#include <QtTypes>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace phvikapen::app {

// One thing kept to be used again.
struct ElementItem {
    QString elementId;
    QString name;
    QString kind;
    QString picture;
    int things{};

    friend bool operator==(const ElementItem&, const ElementItem&) = default;
};

class ElementListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by ElementsViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kElementIdRole = Qt::UserRole + 1;
    static constexpr int kNameRole = Qt::UserRole + 2;
    static constexpr int kKindRole = Qt::UserRole + 3;
    static constexpr int kPictureRole = Qt::UserRole + 4;
    static constexpr int kThingsRole = Qt::UserRole + 5;

    explicit ElementListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<ElementItem> items);

signals:
    void countChanged();

private:
    std::vector<ElementItem> m_items;
};

// Things kept to be used again: a drawing, a shape, a piece of handwriting, a block of type, a
// table. They are kept in a notebook of their own, beside the reader's own notebooks, so that
// everything already written for keeping a page works for keeping an element as well: one kind of
// file, one way of reading it, one place where bytes live.
class ElementsViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(phvikapen::app::ElementListModel* elements READ elements CONSTANT FINAL)
    Q_PROPERTY(QStringList kinds READ kinds NOTIFY elementsChanged FINAL)
    Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY shownChanged FINAL)
    Q_PROPERTY(QString looking READ looking WRITE setLooking NOTIFY shownChanged FINAL)
    Q_PROPERTY(bool ready READ ready NOTIFY elementsChanged FINAL)
    Q_PROPERTY(QString trouble READ trouble NOTIFY troubleChanged FINAL)
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged FINAL)

public:
    // What an element is named when the reader names nothing, and the kind it is put under.
    static constexpr int kLongestName = 80;

    explicit ElementsViewModel(QObject* parent = nullptr);
    ~ElementsViewModel() override;

    ElementsViewModel(const ElementsViewModel&) = delete;
    ElementsViewModel& operator=(const ElementsViewModel&) = delete;
    ElementsViewModel(ElementsViewModel&&) = delete;
    ElementsViewModel& operator=(ElementsViewModel&&) = delete;

    [[nodiscard]] ElementListModel* elements() { return &m_model; }

    [[nodiscard]] QStringList kinds() const;

    [[nodiscard]] QString kind() const { return m_kind; }

    void setKind(const QString& kind);

    [[nodiscard]] QString looking() const { return m_looking; }

    void setLooking(const QString& looking);

    [[nodiscard]] bool ready() const { return m_storage.has_value(); }

    [[nodiscard]] QString trouble() const { return m_trouble; }

    [[nodiscard]] QString directory() const { return m_directory; }

    void setDirectory(const QString& directory);

    // Everything the reader has hold of, kept as an element under a name and a kind.
    Q_INVOKABLE void keep(phvikapen::app::NotebookViewModel* from, const QString& name,
                          const QString& kind);

    // An element put on the page being read, in the middle of what is being looked at.
    Q_INVOKABLE void put(phvikapen::app::NotebookViewModel* into, const QString& elementId);

    Q_INVOKABLE void rename(const QString& elementId, const QString& name);

    Q_INVOKABLE void remove(const QString& elementId);

    Q_INVOKABLE void forgetTrouble();

    // Whether there is anything at all the reader could keep just now.
    Q_INVOKABLE [[nodiscard]] static bool anythingToKeep(phvikapen::app::NotebookViewModel* from);

signals:
    void elementsChanged();
    void shownChanged();
    void troubleChanged();
    void directoryChanged();
    void kept(const QString& elementId);

private:
    // An element as it is drawn: what is on it, and what each of its pictures is made of. The
    // pictures are held beside the page so that they outlive the drawing, which is what the
    // painter is handed pointers to.
    struct Showing {
        core::LoadedPage page;
        std::vector<QImage> images;
    };

    void open();
    void publish();
    void takeTrouble(const QString& why);
    void writeInto(const core::Uuid& elementId, const NotebookViewModel::Handful& handful);
    void drawPicture(const core::Uuid& elementId);
    void fetchThenPaint(const core::Uuid& elementId, const std::shared_ptr<Showing>& showing);
    void paintElement(const core::Uuid& elementId, const std::shared_ptr<Showing>& showing);
    void fetchThenPutDown(const std::shared_ptr<NotebookViewModel::Handful>& handful,
                          const QPointer<NotebookViewModel>& keeper);
    [[nodiscard]] const core::PageInfo* elementNamed(const QString& elementId) const;

    QString m_directory;
    QString m_kind;
    QString m_looking;
    QString m_trouble;
    ElementListModel m_model;
    core::Outline m_outline;
    core::Uuid7Generator m_ids;
    std::optional<core::StorageThread> m_storage;
    std::map<core::Uuid, int> m_pictures;
    int m_pictureRevision{0};
    std::uint64_t m_opening{0};
};

}
