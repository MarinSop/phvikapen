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
#include <optional>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr auto kLayoutSetting = "view/workspace";
constexpr int kDefaultExtent = 240;
constexpr int kSectionsExtent = 160;
constexpr int kMostKinds = 2;
constexpr int kMostWindows = 8;
constexpr int kMostEdges = 3;

struct KnownPanel {
    const char* id;
    bool openAtFirst;
};

constexpr std::array kKnownPanels{
    KnownPanel{.id = "sections", .openAtFirst = true},
    KnownPanel{.id = "pages", .openAtFirst = true},
    KnownPanel{.id = "layers", .openAtFirst = false},
    KnownPanel{.id = "pageSetup", .openAtFirst = false},
    KnownPanel{.id = "maths", .openAtFirst = false},
    KnownPanel{.id = "time", .openAtFirst = false},
    KnownPanel{.id = "sound", .openAtFirst = false},
};

[[nodiscard]] bool isKnownPanel(const QString& panelId) noexcept {
    return std::ranges::any_of(kKnownPanels, [&panelId](const KnownPanel& known) {
        return QString::fromLatin1(known.id) == panelId;
    });
}

[[nodiscard]] QString joined(const QString& path, std::size_t step) {
    const QString here = QString::number(step);
    return path.isEmpty() ? here : path + QLatin1Char{'.'} + here;
}

// A path names a place in the docked tree ("0.1"), or a place inside the window that floats over
// the sheet ("~2.0"). Nothing comes back where the path is not one of those.
struct Trail {
    int afloat{-1};
    QString rest;
};

[[nodiscard]] std::optional<Trail> trailOf(const QString& path) {
    if (!path.startsWith(QLatin1Char{'~'})) {
        return Trail{.afloat = -1, .rest = path};
    }
    const QString held = path.mid(1);
    const qsizetype dot = held.indexOf(QLatin1Char{'.'});
    bool read = false;
    const int at = (dot < 0 ? held : held.left(dot)).toInt(&read);
    if (!read || at < 0) {
        return std::nullopt;
    }
    return Trail{.afloat = at, .rest = dot < 0 ? QString{} : held.mid(dot + 1)};
}

template <typename N>
[[nodiscard]] N* walkDown(N* here, const QString& path) {
    if (path.isEmpty()) {
        return here;
    }
    for (const QString& step : path.split(QLatin1Char{'.'})) {
        bool read = false;
        const int at = step.toInt(&read);
        if (!read || at < 0 || static_cast<std::size_t>(at) >= here->children.size()) {
            return nullptr;
        }
        here = &here->children[static_cast<std::size_t>(at)];
    }
    return here;
}

}

WorkspaceViewModel::WorkspaceViewModel(QObject* parent) : QObject(parent) {
    readLayout();
}

void WorkspaceViewModel::layOutAfresh() {
    Node sections;
    sections.panels = QStringList{QStringLiteral("sections")};
    sections.extent = kSectionsExtent;
    Node pages;
    pages.panels = QStringList{QStringLiteral("pages")};
    Node beside;
    beside.kind = Node::Kind::Split;
    beside.across = false;
    beside.extent = kDefaultExtent;
    beside.children.push_back(std::move(sections));
    beside.children.push_back(std::move(pages));

    Node middle;
    middle.kind = Node::Kind::Middle;
    m_root = Node{};
    m_root.kind = Node::Kind::Split;
    m_root.children.push_back(std::move(beside));
    m_root.children.push_back(std::move(middle));
    m_afloat.clear();
    m_closed.clear();
    for (const KnownPanel& known : kKnownPanels) {
        if (!known.openAtFirst) {
            m_closed.emplace_back(QString::fromLatin1(known.id),
                                  Remembered{.path = QString{}, .edge = Edge::Right});
        }
    }
}

WorkspaceViewModel::Node* WorkspaceViewModel::nodeAt(const QString& path) {
    const std::optional<Trail> trail = trailOf(path);
    if (!trail) {
        return nullptr;
    }
    if (trail->afloat < 0) {
        return walkDown(&m_root, trail->rest);
    }
    if (static_cast<std::size_t>(trail->afloat) >= m_afloat.size()) {
        return nullptr;
    }
    return walkDown(&m_afloat[static_cast<std::size_t>(trail->afloat)].root, trail->rest);
}

const WorkspaceViewModel::Node* WorkspaceViewModel::nodeAt(const QString& path) const {
    const std::optional<Trail> trail = trailOf(path);
    if (!trail) {
        return nullptr;
    }
    if (trail->afloat < 0) {
        return walkDown(&m_root, trail->rest);
    }
    if (static_cast<std::size_t>(trail->afloat) >= m_afloat.size()) {
        return nullptr;
    }
    return walkDown(&m_afloat[static_cast<std::size_t>(trail->afloat)].root, trail->rest);
}

WorkspaceViewModel::Afloat* WorkspaceViewModel::windowAt(const QString& path) {
    const std::optional<Trail> trail = trailOf(path);
    if (!trail || trail->afloat < 0 || static_cast<std::size_t>(trail->afloat) >= m_afloat.size()) {
        return nullptr;
    }
    return &m_afloat[static_cast<std::size_t>(trail->afloat)];
}

QVariantMap WorkspaceViewModel::shapeOf(const Node& node, const QString& path) const {
    QVariantList children;
    for (std::size_t step = 0; step < node.children.size(); ++step) {
        children.append(shapeOf(node.children[step], joined(path, step)));
    }
    return QVariantMap{
        {QStringLiteral("kind"), static_cast<int>(node.kind)},
        {QStringLiteral("path"), path},
        {QStringLiteral("panels"), QVariant{node.panels}},
        {QStringLiteral("current"), node.current},
        {QStringLiteral("across"), node.across},
        {QStringLiteral("extent"), node.extent},
        {QStringLiteral("children"), children},
    };
}

QVariantMap WorkspaceViewModel::layout() const {
    return shapeOf(m_root, QString{});
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

QVariantList WorkspaceViewModel::floating() const {
    QVariantList shown;
    for (std::size_t step = 0; step < m_afloat.size(); ++step) {
        const Afloat& window = m_afloat[step];
        const QString path = QLatin1Char{'~'} + QString::number(step);
        shown.append(QVariantMap{
            {QStringLiteral("path"), path},
            {QStringLiteral("x"), window.x},
            {QStringLiteral("y"), window.y},
            {QStringLiteral("width"), window.width},
            {QStringLiteral("height"), window.height},
            {QStringLiteral("node"), shapeOf(window.root, path)},
        });
    }
    return shown;
}

std::optional<QString> WorkspaceViewModel::pathOfPanel(const QString& panelId) const {
    const auto look = [&panelId](this auto&& again, const Node& node,
                                 const QString& path) -> std::optional<QString> {
        if (node.kind == Node::Kind::Stack && node.panels.contains(panelId)) {
            return path;
        }
        for (std::size_t step = 0; step < node.children.size(); ++step) {
            if (const std::optional<QString> found =
                    again(node.children[step], joined(path, step))) {
                return found;
            }
        }
        return std::nullopt;
    };
    if (const std::optional<QString> found = look(m_root, QString{})) {
        return found;
    }
    for (std::size_t step = 0; step < m_afloat.size(); ++step) {
        if (const std::optional<QString> found =
                look(m_afloat[step].root, QLatin1Char{'~'} + QString::number(step))) {
            return found;
        }
    }
    return std::nullopt;
}

std::optional<QString> WorkspaceViewModel::pathOfMiddle() const {
    const auto look = [](this auto&& again, const Node& node,
                         const QString& path) -> std::optional<QString> {
        if (node.kind == Node::Kind::Middle) {
            return path;
        }
        for (std::size_t step = 0; step < node.children.size(); ++step) {
            if (const std::optional<QString> found =
                    again(node.children[step], joined(path, step))) {
                return found;
            }
        }
        return std::nullopt;
    };
    return look(m_root, QString{});
}

bool WorkspaceViewModel::isOpen(const QString& panelId) const {
    return pathOfPanel(panelId).has_value();
}

void WorkspaceViewModel::takeOut(const QString& panelId) {
    const std::optional<QString> where = pathOfPanel(panelId);
    if (!where) {
        return;
    }
    Node* const stack = nodeAt(*where);
    if (stack == nullptr) {
        return;
    }
    stack->panels.removeAt(stack->panels.indexOf(panelId));
    stack->current =
        std::clamp(stack->current, 0, std::max(0, static_cast<int>(stack->panels.size()) - 1));
    Remembered kept;
    kept.path = *where;
    kept.asTab = !stack->panels.isEmpty();
    if (const Afloat* const window = windowAt(*where)) {
        kept.afloat = true;
        kept.x = window->x;
        kept.y = window->y;
        kept.width = window->width;
        kept.height = window->height;
    }
    std::erase_if(m_closed, [&panelId](const auto& gone) { return gone.first == panelId; });
    m_closed.emplace_back(panelId, kept);
    tidyAll();
}

void WorkspaceViewModel::tidyAll() {
    tidy(m_root);
    for (Afloat& window : m_afloat) {
        tidy(window.root);
    }
    std::erase_if(m_afloat, [](const Afloat& window) {
        return window.root.kind == Node::Kind::Stack && window.root.panels.isEmpty();
    });
}

void WorkspaceViewModel::tidy(Node& node) {
    if (node.kind != Node::Kind::Split) {
        node.current =
            std::clamp(node.current, 0, std::max(0, static_cast<int>(node.panels.size()) - 1));
        return;
    }
    for (Node& child : node.children) {
        tidy(child);
    }
    std::erase_if(node.children, [](const Node& child) {
        return child.kind == Node::Kind::Stack && child.panels.isEmpty();
    });
    if (node.children.size() == 1) {
        const int extent = node.extent;
        Node only = std::move(node.children.front());
        node = std::move(only);
        node.extent = extent;
        return;
    }
    std::vector<Node> flattened;
    for (Node& child : node.children) {
        if (child.kind == Node::Kind::Split && child.across == node.across) {
            for (Node& held : child.children) {
                flattened.push_back(std::move(held));
            }
        } else {
            flattened.push_back(std::move(child));
        }
    }
    node.children = std::move(flattened);
}

void WorkspaceViewModel::settleQuietly() {
    tidyAll();
    writeLayout();
}

void WorkspaceViewModel::settle() {
    settleQuietly();
    emit layoutChanged();
}

void WorkspaceViewModel::openPanel(const QString& panelId) {
    if (!isKnownPanel(panelId) || isOpen(panelId)) {
        return;
    }
    const auto remembered = std::ranges::find_if(
        m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
    const Remembered where = remembered == m_closed.end() ? Remembered{} : remembered->second;
    if (remembered != m_closed.end()) {
        m_closed.erase(remembered);
    }
    if (where.afloat) {
        floatPanel(panelId, where.x, where.y, where.width, where.height);
        return;
    }
    const Node* const back = nodeAt(where.path);
    const bool stackIsStillThere =
        back != nullptr && back->kind == Node::Kind::Stack && !where.path.isEmpty();
    if (stackIsStillThere && where.asTab) {
        dropAsTab(panelId, where.path, static_cast<int>(back->panels.size()));
        return;
    }
    if (stackIsStillThere) {
        dropBeside(panelId, where.path, where.edge);
        return;
    }
    if (const std::optional<QString> middle = pathOfMiddle()) {
        dropBeside(panelId, *middle, where.edge);
    }
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
    const std::optional<QString> where = pathOfPanel(panelId);
    if (!where) {
        openPanel(panelId);
        return;
    }
    if (Node* const stack = nodeAt(*where); stack != nullptr) {
        stack->current = static_cast<int>(stack->panels.indexOf(panelId));
    }
    settleQuietly();
    emit panelShown(panelId);
}

std::optional<QString> WorkspaceViewModel::anchorOf(const QString& path,
                                                    const QString& moving) const {
    const Node* const target = nodeAt(path);
    if (target == nullptr) {
        return std::nullopt;
    }
    if (target->kind == Node::Kind::Middle) {
        return QString{};
    }
    for (const QString& panelId : target->panels) {
        if (panelId != moving) {
            return panelId;
        }
    }
    return std::nullopt;
}

std::optional<QString>
WorkspaceViewModel::pathOfAnchor(const std::optional<QString>& anchor) const {
    if (!anchor) {
        return std::nullopt;
    }
    return anchor->isEmpty() ? pathOfMiddle() : pathOfPanel(*anchor);
}

void WorkspaceViewModel::dropBeside(const QString& panelId, const QString& path, Edge edge) {
    const Node* const target = nodeAt(path);
    if (!isKnownPanel(panelId) || target == nullptr) {
        return;
    }
    const std::optional<QString> stood = pathOfPanel(panelId);
    const bool alreadyAloneThere =
        stood && *stood == path && target->kind == Node::Kind::Stack && target->panels.size() == 1;
    if (alreadyAloneThere) {
        return;
    }
    // Taking the panel out of where it stood can fold a split away, and every path below it then
    // means something else. The place to drop it is held by what stands there, not by its path.
    const std::optional<QString> anchor = anchorOf(path, panelId);
    takeOut(panelId);
    const std::optional<QString> now = pathOfAnchor(anchor);
    Node* const beside = now ? nodeAt(*now) : nullptr;
    if (beside == nullptr) {
        return;
    }
    const bool across = edge == Edge::Left || edge == Edge::Right;
    const bool first = edge == Edge::Left || edge == Edge::Top;
    Node brought;
    brought.panels = QStringList{panelId};
    brought.extent = beside->extent > 0 ? beside->extent / 2 : kDefaultExtent;

    Node split;
    split.kind = Node::Kind::Split;
    split.across = across;
    split.extent = beside->extent;
    Node kept = std::move(*beside);
    kept.extent = 0;
    if (first) {
        split.children.push_back(std::move(brought));
        split.children.push_back(std::move(kept));
    } else {
        split.children.push_back(std::move(kept));
        split.children.push_back(std::move(brought));
    }
    *beside = std::move(split);
    std::erase_if(m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
    settle();
    emit panelShown(panelId);
}

void WorkspaceViewModel::dropAsTab(const QString& panelId, const QString& path, int at) {
    const Node* const target = nodeAt(path);
    if (!isKnownPanel(panelId) || target == nullptr || target->kind != Node::Kind::Stack) {
        return;
    }
    const std::optional<QString> stood = pathOfPanel(panelId);
    if (stood && *stood == path && target->panels.size() == 1) {
        return;
    }
    const std::optional<QString> anchor = anchorOf(path, panelId);
    takeOut(panelId);
    const std::optional<QString> now = pathOfAnchor(anchor);
    Node* const stack = now ? nodeAt(*now) : nullptr;
    if (stack == nullptr || stack->kind != Node::Kind::Stack) {
        return;
    }
    const int place = std::clamp(at, 0, static_cast<int>(stack->panels.size()));
    stack->panels.insert(place, panelId);
    stack->current = place;
    std::erase_if(m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
    settle();
    emit panelShown(panelId);
}

void WorkspaceViewModel::floatPanel(const QString& panelId, int x, int y, int width, int height) {
    if (!isKnownPanel(panelId)) {
        return;
    }
    if (const std::optional<QString> stood = pathOfPanel(panelId)) {
        Afloat* const was = windowAt(*stood);
        if (was != nullptr && was->root.kind == Node::Kind::Stack && was->root.panels.size() == 1) {
            was->x = x;
            was->y = y;
            was->width = std::max(width, kLeastExtent);
            was->height = std::max(height, kLeastExtent);
            settle();
            emit panelShown(panelId);
            return;
        }
    }
    if (m_afloat.size() >= kMostWindows) {
        return;
    }
    takeOut(panelId);
    Afloat window;
    window.root.panels = QStringList{panelId};
    window.x = x;
    window.y = y;
    window.width = std::max(width, kLeastExtent);
    window.height = std::max(height, kLeastExtent);
    m_afloat.push_back(std::move(window));
    std::erase_if(m_closed, [&panelId](const auto& kept) { return kept.first == panelId; });
    settle();
    emit panelShown(panelId);
}

bool WorkspaceViewModel::isAfloat(const QString& panelId) const {
    const std::optional<QString> where = pathOfPanel(panelId);
    return where && where->startsWith(QLatin1Char{'~'});
}

void WorkspaceViewModel::dockPanel(const QString& panelId) {
    if (!isAfloat(panelId)) {
        return;
    }
    const std::optional<QString> middle = pathOfMiddle();
    if (!middle) {
        return;
    }
    dropBeside(panelId, *middle, Edge::Right);
}

void WorkspaceViewModel::movePanelWindow(const QString& path, int x, int y) {
    Afloat* const window = windowAt(path);
    if (window == nullptr || (window->x == x && window->y == y)) {
        return;
    }
    window->x = x;
    window->y = y;
    writeLayout();
}

void WorkspaceViewModel::sizePanelWindow(const QString& path, int width, int height) {
    Afloat* const window = windowAt(path);
    const int wide = std::max(width, kLeastExtent);
    const int tall = std::max(height, kLeastExtent);
    if (window == nullptr || (window->width == wide && window->height == tall)) {
        return;
    }
    window->width = wide;
    window->height = tall;
    writeLayout();
}

void WorkspaceViewModel::choosePanel(const QString& path, int index) {
    Node* const stack = nodeAt(path);
    if (stack == nullptr || stack->kind != Node::Kind::Stack) {
        return;
    }
    if (index < 0 || index >= static_cast<int>(stack->panels.size()) || stack->current == index) {
        return;
    }
    stack->current = index;
    settleQuietly();
}

void WorkspaceViewModel::setExtent(const QString& path, int extent) {
    Node* const node = nodeAt(path);
    const int wanted = std::max(extent, kLeastExtent);
    if (node == nullptr || node->extent == wanted) {
        return;
    }
    node->extent = wanted;
    writeLayout();
}

void WorkspaceViewModel::resetWorkspace() {
    layOutAfresh();
    settle();
}

QJsonObject WorkspaceViewModel::writtenNode(const Node& node) {
    QJsonArray children;
    for (const Node& child : node.children) {
        children.append(writtenNode(child));
    }
    return QJsonObject{
        {QStringLiteral("kind"), static_cast<int>(node.kind)},
        {QStringLiteral("panels"), QJsonArray::fromStringList(node.panels)},
        {QStringLiteral("current"), node.current},
        {QStringLiteral("across"), node.across},
        {QStringLiteral("extent"), node.extent},
        {QStringLiteral("children"), children},
    };
}

void WorkspaceViewModel::writeLayout() const {
    QJsonObject closed;
    for (const auto& [panelId, where] : m_closed) {
        closed.insert(panelId, QJsonObject{
                                   {QStringLiteral("path"), where.path},
                                   {QStringLiteral("edge"), static_cast<int>(where.edge)},
                                   {QStringLiteral("asTab"), where.asTab},
                                   {QStringLiteral("afloat"), where.afloat},
                                   {QStringLiteral("x"), where.x},
                                   {QStringLiteral("y"), where.y},
                                   {QStringLiteral("width"), where.width},
                                   {QStringLiteral("height"), where.height},
                               });
    }
    QJsonArray afloat;
    for (const Afloat& window : m_afloat) {
        afloat.append(QJsonObject{
            {QStringLiteral("x"), window.x},
            {QStringLiteral("y"), window.y},
            {QStringLiteral("width"), window.width},
            {QStringLiteral("height"), window.height},
            {QStringLiteral("root"), writtenNode(window.root)},
        });
    }
    QSettings settings;
    settings.setValue(kLayoutSetting, QString::fromUtf8(QJsonDocument{
                                          QJsonObject{
                                              {QStringLiteral("root"), writtenNode(m_root)},
                                              {QStringLiteral("afloat"), afloat},
                                              {QStringLiteral("closed"), closed},
                                          }}.toJson(QJsonDocument::Compact)));
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

    int middles = 0;
    QStringList seen;
    const auto read = [&middles, &seen](this auto&& again, const QJsonObject& held) -> Node {
        Node node;
        node.kind = static_cast<Node::Kind>(
            std::clamp(held.value(QStringLiteral("kind")).toInt(), 0, kMostKinds));
        node.current = held.value(QStringLiteral("current")).toInt();
        node.across = held.value(QStringLiteral("across")).toBool(true);
        node.extent = held.value(QStringLiteral("extent")).toInt();
        if (node.kind == Node::Kind::Middle) {
            ++middles;
            return node;
        }
        for (const auto& named : held.value(QStringLiteral("panels")).toArray()) {
            const QString panelId = named.toString();
            if (isKnownPanel(panelId) && !seen.contains(panelId)) {
                seen.append(panelId);
                node.panels.append(panelId);
            }
        }
        for (const auto& child : held.value(QStringLiteral("children")).toArray()) {
            node.children.push_back(again(child.toObject()));
        }
        return node;
    };

    Node root = read(kept.value(QStringLiteral("root")).toObject());
    if (middles != 1) {
        return;
    }
    m_root = std::move(root);

    m_afloat.clear();
    for (const auto& held : kept.value(QStringLiteral("afloat")).toArray()) {
        if (m_afloat.size() >= kMostWindows) {
            break;
        }
        const QJsonObject said = held.toObject();
        middles = 0;
        Node standing = read(said.value(QStringLiteral("root")).toObject());
        if (middles != 0) {
            continue;
        }
        Afloat window;
        window.root = std::move(standing);
        window.x = said.value(QStringLiteral("x")).toInt();
        window.y = said.value(QStringLiteral("y")).toInt();
        window.width = std::max(said.value(QStringLiteral("width")).toInt(), kLeastExtent);
        window.height = std::max(said.value(QStringLiteral("height")).toInt(), kLeastExtent);
        m_afloat.push_back(std::move(window));
    }
    tidyAll();

    m_closed.clear();
    const QJsonObject closed = kept.value(QStringLiteral("closed")).toObject();
    for (auto held = closed.begin(); held != closed.end(); ++held) {
        if (!isKnownPanel(held.key()) || seen.contains(held.key())) {
            continue;
        }
        const QJsonObject where = held.value().toObject();
        Remembered back;
        back.path = where.value(QStringLiteral("path")).toString();
        back.edge = static_cast<Edge>(
            std::clamp(where.value(QStringLiteral("edge")).toInt(), 0, kMostEdges));
        back.asTab = where.value(QStringLiteral("asTab")).toBool();
        back.afloat = where.value(QStringLiteral("afloat")).toBool();
        back.x = where.value(QStringLiteral("x")).toInt();
        back.y = where.value(QStringLiteral("y")).toInt();
        back.width = std::max(where.value(QStringLiteral("width")).toInt(), kLeastExtent);
        back.height = std::max(where.value(QStringLiteral("height")).toInt(), kLeastExtent);
        m_closed.emplace_back(held.key(), back);
    }
}

}
