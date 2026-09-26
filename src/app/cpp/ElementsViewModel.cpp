#include "app/cpp/ElementsViewModel.hpp"

#include "app/cpp/NotebookViewModel.hpp"
#include "app/cpp/Thumbnails.hpp"
#include "core/Error.hpp"
#include "core/ink/StrokeSelection.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "platform/render/PagePainter.hpp"
#include "platform/render/PaperLook.hpp"

#include <QColor>
#include <QDir>
#include <QImage>
#include <QMetaObject>
#include <QPainter>
#include <QPointer>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr auto kLibraryName = "elements.phvika";
constexpr auto kPlainKind = "Everything";
constexpr float kEmptyRatio = 0.75F;
constexpr float kRoomAround = 8.0F;

[[nodiscard]] QString plainPlace() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/elements");
}

// What the writer is handed to put a new kind of element down, and a new element in a kind that
// is already there. Both are held by a pointer, so that handing them over cannot fail.
struct Making {
    core::SectionInfo section;
    std::vector<core::Uuid> order;
    std::vector<core::Uuid> inside;
};

struct Adding {
    core::Uuid section;
    core::PageInfo page;
    std::vector<core::Uuid> order;
};

[[nodiscard]] bool holdsWord(const QString& said, const QString& wanted) {
    return wanted.isEmpty() || said.contains(wanted, Qt::CaseInsensitive);
}

// The box around everything a handful holds, so that it can be put down in the middle of what is
// being looked at rather than where it happened to stand.
[[nodiscard]] core::Rect areaAround(const NotebookViewModel::Handful& handful) {
    std::optional<core::Rect> around;
    const auto widen = [&around](const core::Rect& one) {
        around = around ? around->united(one) : one;
    };
    for (const core::PlacedStroke& placed : handful.strokes) {
        if (const std::optional<core::Rect> bounds = placed.stroke.boundingBox()) {
            widen(*bounds);
        }
    }
    for (const core::PlacedText& placed : handful.texts) {
        widen(core::areaOf(placed.box));
    }
    for (const core::PlacedTable& placed : handful.tables) {
        widen(core::areaOf(placed.table));
    }
    return around.value_or(core::Rect{});
}

}

ElementListModel::ElementListModel(QObject* parent) : QAbstractListModel(parent) {}

int ElementListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant ElementListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const ElementItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kNameRole:
        return item.name;
    case kElementIdRole:
        return item.elementId;
    case kKindRole:
        return item.kind;
    case kPictureRole:
        return item.picture;
    case kThingsRole:
        return item.things;
    default:
        return {};
    }
}

QHash<int, QByteArray> ElementListModel::roleNames() const {
    return {
        {kElementIdRole, "elementId"}, {kNameRole, "name"},     {kKindRole, "kind"},
        {kPictureRole, "picture"},     {kThingsRole, "things"},
    };
}

void ElementListModel::setItems(std::vector<ElementItem> items) {
    if (items == m_items) {
        return;
    }
    const bool sameCount = items.size() == m_items.size();
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    if (!sameCount) {
        emit countChanged();
    }
}

ElementsViewModel::ElementsViewModel(QObject* parent) : QObject(parent), m_directory{plainPlace()} {
    open();
}

ElementsViewModel::~ElementsViewModel() = default;

void ElementsViewModel::setDirectory(const QString& directory) {
    if (directory == m_directory) {
        return;
    }
    m_directory = directory;
    emit directoryChanged();
    open();
}

void ElementsViewModel::takeTrouble(const QString& why) {
    m_trouble = why;
    emit troubleChanged();
}

void ElementsViewModel::forgetTrouble() {
    if (m_trouble.isEmpty()) {
        return;
    }
    m_trouble.clear();
    emit troubleChanged();
}

void ElementsViewModel::open() {
    ++m_opening;
    m_storage.reset();
    m_outline = core::Outline{};
    m_pictures.clear();
    if (!QDir{}.mkpath(m_directory)) {
        takeTrouble(tr("The elements could not be kept in %1").arg(m_directory));
        publish();
        return;
    }
    const std::filesystem::path where =
        std::filesystem::path{m_directory.toStdString()} / kLibraryName;
    m_storage.emplace(where, [this](const core::Error& error) {
        QMetaObject::invokeMethod(
            this, [this, message = QString::fromStdString(error.message)] { takeTrouble(message); },
            Qt::QueuedConnection);
    });
    const std::uint64_t opening = m_opening;
    m_storage->loadOutline([this, opening](core::Result<core::NotebookOutline> outline) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, outline = std::move(outline)] {
                if (opening != m_opening) {
                    return;
                }
                if (!outline) {
                    takeTrouble(QString::fromStdString(outline.error().message));
                    return;
                }
                m_outline = core::Outline{*outline};
                publish();
            },
            Qt::QueuedConnection);
    });
    publish();
}

QStringList ElementsViewModel::kinds() const {
    QStringList said{tr(kPlainKind)};
    for (const core::SectionInfo& section : m_outline.sections()) {
        const QString kind = QString::fromStdString(section.title);
        if (!kind.isEmpty() && !said.contains(kind)) {
            said.append(kind);
        }
    }
    return said;
}

void ElementsViewModel::setKind(const QString& kind) {
    if (kind == m_kind) {
        return;
    }
    m_kind = kind;
    emit shownChanged();
    publish();
}

void ElementsViewModel::setLooking(const QString& looking) {
    if (looking == m_looking) {
        return;
    }
    m_looking = looking;
    emit shownChanged();
    publish();
}

const core::PageInfo* ElementsViewModel::elementNamed(const QString& elementId) const {
    for (const core::SectionInfo& section : m_outline.sections()) {
        for (const core::PageInfo& page : section.pages) {
            if (QString::fromStdString(page.id.toString()) == elementId) {
                return &page;
            }
        }
    }
    return nullptr;
}

void ElementsViewModel::publish() {
    std::vector<ElementItem> items;
    const QString plain = tr(kPlainKind);
    for (const core::SectionInfo& section : m_outline.sections()) {
        const QString kind = QString::fromStdString(section.title);
        if (!m_kind.isEmpty() && m_kind != plain && m_kind != kind) {
            continue;
        }
        for (const core::PageInfo& page : section.pages) {
            const QString name = QString::fromStdString(page.title);
            if (!holdsWord(name, m_looking) && !holdsWord(kind, m_looking)) {
                continue;
            }
            const auto drawn = m_pictures.find(page.id);
            items.push_back(ElementItem{
                .elementId = QString::fromStdString(page.id.toString()),
                .name = name,
                .kind = kind,
                .picture = drawn == m_pictures.end()
                               ? QString{}
                               : QStringLiteral("image://pages/element-%1-%2")
                                     .arg(QString::fromStdString(page.id.toString()))
                                     .arg(drawn->second),
                .things = 0,
            });
        }
    }
    m_model.setItems(std::move(items));
    emit elementsChanged();
}

bool ElementsViewModel::anythingToKeep(NotebookViewModel* from) {
    if (from == nullptr) {
        return false;
    }
    const NotebookViewModel::Handful handful = from->handfulPicked();
    return !handful.strokes.empty() || !handful.texts.empty() || !handful.tables.empty();
}

void ElementsViewModel::keep(NotebookViewModel* from, const QString& name, const QString& kind) {
    if (from == nullptr || !m_storage) {
        takeTrouble(tr("There is nowhere to keep an element."));
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    const NotebookViewModel::Handful handful = from->handfulPicked();
    if (handful.strokes.empty() && handful.texts.empty() && handful.tables.empty()) {
        takeTrouble(tr("Pick up what is to be kept first."));
        return;
    }
    forgetTrouble();

    const QString wanted = kind.trimmed().isEmpty() ? tr("Elements") : kind.trimmed();
    const core::SectionInfo* standing = nullptr;
    for (const core::SectionInfo& section : m_outline.sections()) {
        if (QString::fromStdString(section.title) == wanted) {
            standing = &section;
        }
    }
    QString given = name.trimmed();
    if (given.isEmpty()) {
        given = tr("Element %1").arg(m_model.rowCount() + 1);
    }
    if (given.size() > kLongestName) {
        given.truncate(kLongestName);
    }

    core::PageInfo page{
        .id = m_ids.next(),
        .title = given.toStdString(),
        .style = {},
        .media = std::nullopt,
    };
    const core::Uuid elementId = page.id;

    if (standing == nullptr) {
        core::SectionInfo section{
            .id = m_ids.next(),
            .title = wanted.toStdString(),
            .pages = {page},
        };
        const std::size_t at = m_outline.sections().size();
        if (const core::Result<void> made = m_outline.insertSection(at, section); !made) {
            takeTrouble(QString::fromStdString(made.error().message));
            return;
        }
        // Held by a pointer the writer can take along without any chance of failing.
        const auto made = std::make_shared<const Making>(Making{
            .section = section,
            .order = m_outline.sectionOrder(),
            .inside = {elementId},
        });
        storage->submit([made](core::NotebookStore& store) {
            const core::Result<void> put =
                store.insertSection(made->section.id, made->section.title, made->order);
            return put ? store.insertPages(made->section.id, made->section.pages, made->inside)
                       : put;
        });
    } else {
        const core::Uuid sectionId = standing->id;
        const auto after = standing->pages.size();
        if (const core::Result<void> put =
                m_outline.insertPage(core::PagePlace{.sectionId = sectionId, .index = after}, page);
            !put) {
            takeTrouble(QString::fromStdString(put.error().message));
            return;
        }
        const auto made = std::make_shared<const Adding>(Adding{
            .section = sectionId,
            .page = page,
            .order = m_outline.pageOrder(sectionId),
        });
        storage->submit([made](core::NotebookStore& store) {
            return store.insertPage(made->section, made->page, made->order);
        });
    }

    // Everything is carried to the top left corner, so an element is kept without the room that
    // happened to be around it on the page it came from.
    const float across = -handful.area.left + kRoomAround;
    const float down = -handful.area.top + kRoomAround;
    for (const core::PlacedStroke& placed : handful.strokes) {
        core::PlacedStroke shifted{
            .ordinal = placed.ordinal,
            .stroke = core::moved(placed.stroke, across, down),
            .layer = core::Uuid{},
        };
        storage->submit([elementId, shifted](core::NotebookStore& store) {
            return store.insertStroke(elementId, shifted);
        });
    }
    for (const core::PlacedText& placed : handful.texts) {
        core::PlacedText shifted = placed;
        shifted.box.at.x += across;
        shifted.box.at.y += down;
        shifted.layer = core::Uuid{};
        storage->submit([elementId, shifted](core::NotebookStore& store) {
            return store.insertText(elementId, shifted);
        });
    }
    for (const core::PlacedTable& placed : handful.tables) {
        core::PlacedTable shifted = placed;
        shifted.table.at.x += across;
        shifted.table.at.y += down;
        shifted.layer = core::Uuid{};
        storage->submit([elementId, shifted](core::NotebookStore& store) {
            return store.insertTable(elementId, shifted);
        });
    }

    drawPicture(elementId);
    publish();
    emit kept(QString::fromStdString(elementId.toString()));
}

void ElementsViewModel::drawPicture(const core::Uuid& elementId) {
    if (!m_storage) {
        return;
    }
    const std::uint64_t opening = m_opening;
    core::StorageThread* const storage = &*m_storage;
    storage->loadPage(elementId, [this, opening, elementId](core::Result<core::LoadedPage> got) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, elementId, got = std::move(got)] mutable {
                if (opening != m_opening || !got) {
                    return;
                }
                const platform::render::PageContents contents{
                    .style = {},
                    .strokes = got->strokes,
                    .texts = got->texts,
                    .pictures = {},
                    .tables = got->tables,
                    .layers = {},
                    .media = nullptr,
                };
                const core::Rect area = platform::render::pageArea(contents);
                const bool anything = area.width() > 0.0F && area.height() > 0.0F;
                const float ratio = anything ? area.height() / area.width() : kEmptyRatio;
                const int height =
                    std::max(1, static_cast<int>(static_cast<float>(thumbnails::kWidth) * ratio));
                QImage picture{thumbnails::kWidth, height, QImage::Format_ARGB32_Premultiplied};
                picture.fill(Qt::transparent);
                if (anything) {
                    QPainter painter{&picture};
                    painter.scale(static_cast<double>(thumbnails::kWidth)
                                      / static_cast<double>(area.width()),
                                  static_cast<double>(height) / static_cast<double>(area.height()));
                    platform::render::paintPage(painter, contents, area);
                }
                const int revision = ++m_pictureRevision;
                m_pictures[elementId] = revision;
                thumbnails::put(QStringLiteral("element-%1-%2")
                                    .arg(QString::fromStdString(elementId.toString()))
                                    .arg(revision),
                                picture);
                publish();
            },
            Qt::QueuedConnection);
    });
}

void ElementsViewModel::put(NotebookViewModel* into, const QString& elementId) {
    const core::PageInfo* const element = elementNamed(elementId);
    if (into == nullptr || element == nullptr || !m_storage) {
        takeTrouble(tr("This element could not be found."));
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    forgetTrouble();
    const std::uint64_t opening = m_opening;
    const core::Uuid wanted = element->id;
    QPointer<NotebookViewModel> keeper{into};
    storage->loadPage(wanted, [this, opening, keeper](core::Result<core::LoadedPage> got) {
        QMetaObject::invokeMethod(
            this,
            [this, opening, keeper, got = std::move(got)] mutable {
                if (opening != m_opening || keeper.isNull()) {
                    return;
                }
                if (!got) {
                    takeTrouble(QString::fromStdString(got.error().message));
                    return;
                }
                NotebookViewModel::Handful handful;
                handful.strokes = std::move(got->strokes);
                handful.texts = std::move(got->texts);
                handful.tables = std::move(got->tables);
                handful.area = areaAround(handful);
                keeper->putDownHandful(handful);
            },
            Qt::QueuedConnection);
    });
}

void ElementsViewModel::rename(const QString& elementId, const QString& name) {
    const core::PageInfo* const element = elementNamed(elementId);
    QString given = name.trimmed();
    if (element == nullptr || given.isEmpty() || !m_storage) {
        return;
    }
    if (given.size() > kLongestName) {
        given.truncate(kLongestName);
    }
    const core::Uuid wanted = element->id;
    const std::string said = given.toStdString();
    if (const core::Result<std::string> named = m_outline.renamePage(wanted, said); !named) {
        return;
    }
    core::StorageThread* const storage = &*m_storage;
    const auto title = std::make_shared<const std::string>(said);
    storage->submit(
        [wanted, title](core::NotebookStore& store) { return store.renamePage(wanted, *title); });
    publish();
}

void ElementsViewModel::remove(const QString& elementId) {
    const core::PageInfo* const element = elementNamed(elementId);
    if (element == nullptr || !m_storage) {
        return;
    }
    const core::Uuid wanted = element->id;
    core::StorageThread* const storage = &*m_storage;
    if (const core::Result<core::RemovedPage> gone = m_outline.removePage(wanted); !gone) {
        takeTrouble(QString::fromStdString(gone.error().message));
        return;
    }
    m_pictures.erase(wanted);
    thumbnails::forget(QStringLiteral("element-%1").arg(QString::fromStdString(wanted.toString())));
    storage->submit([wanted](core::NotebookStore& store) { return store.trashPage(wanted); });
    publish();
}

}
