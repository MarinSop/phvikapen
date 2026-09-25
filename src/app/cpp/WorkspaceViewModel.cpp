#include "app/cpp/WorkspaceViewModel.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr auto kLayoutSetting = "view/workspace";
constexpr int kSides = 4;
constexpr int kDefaultSideExtent = 260;

struct KnownPanel {
    const char* id;
    WorkspaceViewModel::Side side;
    bool openAtFirst;
};

constexpr std::array kKnownPanels{
    KnownPanel{.id = "contents", .side = WorkspaceViewModel::Side::Left, .openAtFirst = true},
    KnownPanel{.id = "layers", .side = WorkspaceViewModel::Side::Right, .openAtFirst = false},
    KnownPanel{.id = "pageSetup", .side = WorkspaceViewModel::Side::Right, .openAtFirst = false},
};

[[nodiscard]] const KnownPanel* knownPanel(const QString& panelId) noexcept {
    for (const KnownPanel& known : kKnownPanels) {
        if (QString::fromLatin1(known.id) == panelId) {
            return &known;
        }
    }
    return nullptr;
}

[[nodiscard]] bool isKnownSide(int side) noexcept {
    return side >= 0 && side < kSides;
}

}

WorkspaceViewModel::WorkspaceViewModel(QObject* parent) : QObject(parent) {
    readLayout();
}

std::size_t WorkspaceViewModel::placeOfSide(Side side) noexcept {
    return static_cast<std::size_t>(side);
}

WorkspaceViewModel::Dock& WorkspaceViewModel::dockOf(Side side) noexcept {
    return m_docks[placeOfSide(side)];
}

const WorkspaceViewModel::Dock& WorkspaceViewModel::dockOf(Side side) const noexcept {
    return m_docks[placeOfSide(side)];
}

void WorkspaceViewModel::layOutAfresh() {
    m_docks.clear();
    m_docks.reserve(kSides);
    for (int side = 0; side < kSides; ++side) {
        m_docks.push_back(Dock{
            .side = static_cast<Side>(side),
            .extent = kDefaultSideExtent,
            .groups = {},
        });
    }
    m_closed.clear();
    for (const KnownPanel& known : kKnownPanels) {
        const QString panelId = QString::fromLatin1(known.id);
        if (known.openAtFirst) {
            putIn(panelId, known.side, -1);
        } else {
            m_closed.emplace_back(panelId, Remembered{.side = known.side, .group = 0});
        }
    }
}

QVariantList WorkspaceViewModel::docks() const {
    QVariantList shown;
    for (const Dock& dock : m_docks) {
        QVariantList groups;
        for (const Group& group : dock.groups) {
            groups.append(QVariantMap{
                {QStringLiteral("panels"), QVariant{group.panels}},
                {QStringLiteral("current"), group.current},
                {QStringLiteral("extent"), group.extent},
            });
        }
        shown.append(QVariantMap{
            {QStringLiteral("side"), static_cast<int>(dock.side)},
            {QStringLiteral("extent"), dock.extent},
            {QStringLiteral("groups"), groups},
        });
    }
    return shown;
}

QVariantList WorkspaceViewModel::panels() const {
    QVariantList shown;
    for (const KnownPanel& known : kKnownPanels) {
        const QString panelId = QString::fromLatin1(known.id);
        shown.append(QVariantMap{
            {QStringLiteral("panelId"), panelId},
            {QStringLiteral("open"), isOpen(panelId)},
        });
    }
    return shown;
}

bool WorkspaceViewModel::isOpen(const QString& panelId) const {
    for (const Dock& dock : m_docks) {
        for (const Group& group : dock.groups) {
            if (group.panels.contains(panelId)) {
                return true;
            }
        }
    }
    return false;
}

void WorkspaceViewModel::takeOut(const QString& panelId) {
    for (Dock& dock : m_docks) {
        for (std::size_t at = 0; at < dock.groups.size(); ++at) {
            Group& group = dock.groups[at];
            const qsizetype place = group.panels.indexOf(panelId);
            if (place < 0) {
                continue;
            }
            group.panels.removeAt(place);
            group.current = std::clamp(group.current, 0,
                                       std::max(0, static_cast<int>(group.panels.size()) - 1));
            std::erase_if(m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
            m_closed.emplace_back(panelId, Remembered{
                                               .side = dock.side,
                                               .group = static_cast<int>(at),
                                           });
            return;
        }
    }
}

void WorkspaceViewModel::putIn(const QString& panelId, Side side, int group) {
    Dock& dock = dockOf(side);
    if (group >= 0 && static_cast<std::size_t>(group) < dock.groups.size()) {
        Group& into = dock.groups[static_cast<std::size_t>(group)];
        into.panels.append(panelId);
        into.current = static_cast<int>(into.panels.size()) - 1;
        return;
    }
    dock.groups.push_back(Group{
        .panels = QStringList{panelId},
        .current = 0,
        .extent = 0,
    });
}

void WorkspaceViewModel::tidy() {
    for (Dock& dock : m_docks) {
        std::erase_if(dock.groups, [](const Group& group) { return group.panels.isEmpty(); });
        for (Group& group : dock.groups) {
            group.current = std::clamp(group.current, 0, static_cast<int>(group.panels.size()) - 1);
        }
        dock.extent = std::clamp(dock.extent, kLeastExtent, kMostExtent);
    }
}

void WorkspaceViewModel::settleQuietly() {
    tidy();
    writeLayout();
}

void WorkspaceViewModel::settle() {
    settleQuietly();
    emit layoutChanged();
}

void WorkspaceViewModel::openPanel(const QString& panelId) {
    const KnownPanel* const known = knownPanel(panelId);
    if (known == nullptr || isOpen(panelId)) {
        return;
    }
    const auto remembered = std::ranges::find_if(
        m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
    const Remembered where = remembered == m_closed.end()
                                 ? Remembered{.side = known->side, .group = 0}
                                 : remembered->second;
    if (remembered != m_closed.end()) {
        m_closed.erase(remembered);
    }
    const Dock& dock = dockOf(where.side);
    const bool joinsAGroup =
        where.group >= 0 && static_cast<std::size_t>(where.group) < dock.groups.size();
    putIn(panelId, where.side, joinsAGroup ? where.group : -1);
    settle();
    emit panelShown(panelId);
}

void WorkspaceViewModel::closePanel(const QString& panelId) {
    if (!isOpen(panelId)) {
        return;
    }
    takeOut(panelId);
    settle();
}

void WorkspaceViewModel::togglePanel(const QString& panelId) {
    if (isOpen(panelId)) {
        closePanel(panelId);
    } else {
        openPanel(panelId);
    }
}

void WorkspaceViewModel::showPanel(const QString& panelId) {
    if (!isOpen(panelId)) {
        openPanel(panelId);
        return;
    }
    for (Dock& dock : m_docks) {
        for (Group& group : dock.groups) {
            const qsizetype place = group.panels.indexOf(panelId);
            if (place >= 0) {
                group.current = static_cast<int>(place);
            }
        }
    }
    settleQuietly();
    emit panelShown(panelId);
}

void WorkspaceViewModel::dockPanel(const QString& panelId, Side side, int group) {
    if (knownPanel(panelId) == nullptr) {
        return;
    }
    const Dock& before = dockOf(side);
    const bool joinsAGroup = group >= 0 && static_cast<std::size_t>(group) < before.groups.size();
    const bool alreadyThere =
        joinsAGroup && before.groups[static_cast<std::size_t>(group)].panels.contains(panelId);
    if (alreadyThere && before.groups[static_cast<std::size_t>(group)].panels.size() == 1) {
        return;
    }
    takeOut(panelId);
    tidy();
    const Dock& after = dockOf(side);
    const bool stillThere = group >= 0 && static_cast<std::size_t>(group) < after.groups.size();
    putIn(panelId, side, joinsAGroup && stillThere ? group : -1);
    std::erase_if(m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
    settle();
    emit panelShown(panelId);
}

void WorkspaceViewModel::choosePanel(Side side, int group, int index) {
    Dock& dock = dockOf(side);
    if (group < 0 || static_cast<std::size_t>(group) >= dock.groups.size()) {
        return;
    }
    Group& chosen = dock.groups[static_cast<std::size_t>(group)];
    if (index < 0 || index >= static_cast<int>(chosen.panels.size()) || chosen.current == index) {
        return;
    }
    chosen.current = index;
    settleQuietly();
}

void WorkspaceViewModel::setSideExtent(Side side, int extent) {
    Dock& dock = dockOf(side);
    const int wanted = std::clamp(extent, kLeastExtent, kMostExtent);
    if (dock.extent == wanted) {
        return;
    }
    dock.extent = wanted;
    writeLayout();
    emit layoutChanged();
}

void WorkspaceViewModel::setGroupExtent(Side side, int group, int extent) {
    Dock& dock = dockOf(side);
    if (group < 0 || static_cast<std::size_t>(group) >= dock.groups.size()) {
        return;
    }
    Group& chosen = dock.groups[static_cast<std::size_t>(group)];
    const int wanted = std::max(extent, kLeastGroupExtent);
    if (chosen.extent == wanted) {
        return;
    }
    chosen.extent = wanted;
    writeLayout();
}

void WorkspaceViewModel::resetWorkspace() {
    layOutAfresh();
    settle();
}

void WorkspaceViewModel::readLayout() {
    layOutAfresh();
    const QSettings settings;
    const QJsonDocument written =
        QJsonDocument::fromJson(settings.value(kLayoutSetting).toString().toUtf8());
    if (!written.isObject()) {
        return;
    }
    const QJsonObject kept = written.object();
    const QJsonArray docks = kept.value(QStringLiteral("docks")).toArray();
    if (docks.isEmpty()) {
        return;
    }
    for (Dock& dock : m_docks) {
        dock.groups.clear();
    }
    m_closed.clear();
    for (const auto& value : docks) {
        const QJsonObject side = value.toObject();
        const int at = side.value(QStringLiteral("side")).toInt(-1);
        if (!isKnownSide(at)) {
            continue;
        }
        Dock& dock = dockOf(static_cast<Side>(at));
        dock.extent = side.value(QStringLiteral("extent")).toInt(kDefaultSideExtent);
        for (const auto& held : side.value(QStringLiteral("groups")).toArray()) {
            const QJsonObject group = held.toObject();
            QStringList panels;
            for (const auto& named : group.value(QStringLiteral("panels")).toArray()) {
                const QString panelId = named.toString();
                if (knownPanel(panelId) != nullptr && !isOpen(panelId)) {
                    panels.append(panelId);
                }
            }
            if (!panels.isEmpty()) {
                dock.groups.push_back(Group{
                    .panels = std::move(panels),
                    .current = group.value(QStringLiteral("current")).toInt(),
                    .extent = group.value(QStringLiteral("extent")).toInt(),
                });
            }
        }
    }
    const QJsonObject closed = kept.value(QStringLiteral("closed")).toObject();
    for (auto held = closed.begin(); held != closed.end(); ++held) {
        const int side = held.value().toObject().value(QStringLiteral("side")).toInt(-1);
        if (knownPanel(held.key()) != nullptr && isKnownSide(side)) {
            m_closed.emplace_back(
                held.key(),
                Remembered{
                    .side = static_cast<Side>(side),
                    .group = held.value().toObject().value(QStringLiteral("group")).toInt(),
                });
        }
    }
    tidy();
}

void WorkspaceViewModel::writeLayout() const {
    QJsonArray docks;
    for (const Dock& dock : m_docks) {
        QJsonArray groups;
        for (const Group& group : dock.groups) {
            groups.append(QJsonObject{
                {QStringLiteral("panels"), QJsonArray::fromStringList(group.panels)},
                {QStringLiteral("current"), group.current},
                {QStringLiteral("extent"), group.extent},
            });
        }
        docks.append(QJsonObject{
            {QStringLiteral("side"), static_cast<int>(dock.side)},
            {QStringLiteral("extent"), dock.extent},
            {QStringLiteral("groups"), groups},
        });
    }
    QJsonObject closed;
    for (const auto& [panelId, where] : m_closed) {
        closed.insert(panelId, QJsonObject{
                                   {QStringLiteral("side"), static_cast<int>(where.side)},
                                   {QStringLiteral("group"), where.group},
                               });
    }
    QSettings settings;
    settings.setValue(kLayoutSetting, QString::fromUtf8(QJsonDocument{
                                          QJsonObject{
                                              {QStringLiteral("docks"), docks},
                                              {QStringLiteral("closed"), closed},
                                          }}.toJson(QJsonDocument::Compact)));
}

}
