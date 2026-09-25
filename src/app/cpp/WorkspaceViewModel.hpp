#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>

#include <optional>
#include <utility>
#include <vector>

namespace phvikapen::app {

class WorkspaceViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap layout READ layout NOTIFY layoutChanged FINAL)
    Q_PROPERTY(QVariantList panels READ panels NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int leastExtent READ leastExtent CONSTANT FINAL)

public:
    enum class Edge : quint8 { Left, Right, Top, Bottom };
    Q_ENUM(Edge)

    static constexpr int kLeastExtent = 120;

    explicit WorkspaceViewModel(QObject* parent = nullptr);

    [[nodiscard]] QVariantMap layout() const;

    [[nodiscard]] QVariantList panels() const;

    [[nodiscard]] static int leastExtent() { return kLeastExtent; }

    Q_INVOKABLE [[nodiscard]] bool isOpen(const QString& panelId) const;

    Q_INVOKABLE void openPanel(const QString& panelId);

    Q_INVOKABLE void closePanel(const QString& panelId);

    Q_INVOKABLE void togglePanel(const QString& panelId);

    Q_INVOKABLE void showPanel(const QString& panelId);

    Q_INVOKABLE void dropBeside(const QString& panelId, const QString& path, Edge edge);

    Q_INVOKABLE void dropAsTab(const QString& panelId, const QString& path, int at);

    Q_INVOKABLE void choosePanel(const QString& path, int index);

    Q_INVOKABLE void setExtent(const QString& path, int extent);

    Q_INVOKABLE void resetWorkspace();

signals:
    void layoutChanged();
    void panelShown(const QString& panelId);

private:
    struct Node {
        enum class Kind : std::uint8_t { Middle, Stack, Split };

        Kind kind{Kind::Stack};
        QStringList panels;
        int current{0};
        bool across{true};
        int extent{0};
        std::vector<Node> children;
    };

    struct Remembered {
        QString path;
        Edge edge{Edge::Right};
        bool asTab{false};
    };

    [[nodiscard]] Node* nodeAt(const QString& path);
    [[nodiscard]] const Node* nodeAt(const QString& path) const;
    [[nodiscard]] QVariantMap shapeOf(const Node& node, const QString& path) const;
    [[nodiscard]] std::optional<QString> pathOfPanel(const QString& panelId) const;
    [[nodiscard]] std::optional<QString> pathOfMiddle() const;
    [[nodiscard]] std::optional<QString> anchorOf(const QString& path, const QString& moving) const;
    [[nodiscard]] std::optional<QString> pathOfAnchor(const std::optional<QString>& anchor) const;
    [[nodiscard]] static QJsonObject writtenNode(const Node& node);

    void takeOut(const QString& panelId);
    void tidy(Node& node);
    void settle();
    void settleQuietly();
    void readLayout();
    void writeLayout() const;
    void layOutAfresh();

    Node m_root;
    std::vector<std::pair<QString, Remembered>> m_closed;
};

}
