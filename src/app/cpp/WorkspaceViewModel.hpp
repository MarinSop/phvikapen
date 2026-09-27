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
    Q_PROPERTY(QVariantList floating READ floating NOTIFY layoutChanged FINAL)
    Q_PROPERTY(QVariantList looseWindows READ looseWindows NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int leastExtent READ leastExtent CONSTANT FINAL)

public:
    enum class Edge : quint8 { Left, Right, Top, Bottom };
    Q_ENUM(Edge)

    static constexpr int kLeastExtent = 120;
    static constexpr int kWindowWidth = 300;
    static constexpr int kWindowHeight = 340;

    explicit WorkspaceViewModel(QObject* parent = nullptr);

    [[nodiscard]] QVariantMap layout() const;

    [[nodiscard]] QVariantList panels() const;

    [[nodiscard]] QVariantList floating() const;
    [[nodiscard]] QVariantList looseWindows() const;

    // Which panel names a floating window: the first one in it, wherever it sits inside.
    Q_INVOKABLE [[nodiscard]] QString firstPanelOf(const QString& path) const;

    [[nodiscard]] static int leastExtent() { return kLeastExtent; }

    Q_INVOKABLE [[nodiscard]] bool isOpen(const QString& panelId) const;

    Q_INVOKABLE void openPanel(const QString& panelId);

    Q_INVOKABLE void closePanel(const QString& panelId);

    Q_INVOKABLE void togglePanel(const QString& panelId);

    Q_INVOKABLE void showPanel(const QString& panelId);

    Q_INVOKABLE void dropBeside(const QString& panelId, const QString& path, Edge edge);

    Q_INVOKABLE void dropAsTab(const QString& panelId, const QString& path, int at);

    Q_INVOKABLE void floatPanel(const QString& panelId, int x, int y, int width, int height);

    Q_INVOKABLE void dockPanel(const QString& panelId);

    Q_INVOKABLE [[nodiscard]] bool isAfloat(const QString& panelId) const;

    Q_INVOKABLE void movePanelWindow(const QString& path, int x, int y);

    Q_INVOKABLE void sizePanelWindow(const QString& path, int width, int height);

    // Whether a floating panel lives in a window of its own rather than over the sheet. A window
    // of its own can be carried anywhere, including onto another screen.
    Q_INVOKABLE void setPanelLoose(const QString& path, bool loose);

    Q_INVOKABLE [[nodiscard]] bool isLoose(const QString& panelId) const;

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
        bool afloat{false};
        int x{};
        int y{};
        int width{kWindowWidth};
        int height{kWindowHeight};
    };

    struct Afloat {
        Node root;
        int x{};
        int y{};
        int width{kWindowWidth};
        int height{kWindowHeight};
        // A panel set free lives in a window of its own, which can be carried onto another screen.
        // One that is not stands over the sheet, inside the application's own window.
        bool loose{};
    };

    [[nodiscard]] Node* nodeAt(const QString& path);
    [[nodiscard]] const Node* nodeAt(const QString& path) const;
    [[nodiscard]] Afloat* windowAt(const QString& path);
    [[nodiscard]] const Afloat* windowAt(const QString& path) const;
    [[nodiscard]] QVariantList shownAfloat(bool loose) const;
    [[nodiscard]] QVariantMap shapeOf(const Node& node, const QString& path) const;
    [[nodiscard]] std::optional<QString> pathOfPanel(const QString& panelId) const;
    [[nodiscard]] std::optional<QString> pathOfMiddle() const;
    [[nodiscard]] std::optional<QString> anchorOf(const QString& path, const QString& moving) const;
    [[nodiscard]] std::optional<QString> pathOfAnchor(const std::optional<QString>& anchor) const;
    [[nodiscard]] static QJsonObject writtenNode(const Node& node);

    void takeOut(const QString& panelId);
    void tidy(Node& node);
    void tidyAll();
    void settle();
    void settleQuietly();
    void readLayout();
    void writeLayout() const;
    void layOutAfresh();

    Node m_root;
    std::vector<Afloat> m_afloat;
    std::vector<std::pair<QString, Remembered>> m_closed;
};

}
