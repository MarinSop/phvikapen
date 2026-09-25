#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace phvikapen::app {

class WorkspaceViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList docks READ docks NOTIFY layoutChanged FINAL)
    Q_PROPERTY(QVariantList panels READ panels NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int leastGroupExtent READ leastGroupExtent CONSTANT FINAL)

public:
    enum class Side : std::uint8_t { Left, Right, Top, Bottom };
    Q_ENUM(Side)

    static constexpr int kLeastExtent = 140;
    static constexpr int kMostExtent = 640;
    static constexpr int kLeastGroupExtent = 90;

    explicit WorkspaceViewModel(QObject* parent = nullptr);

    [[nodiscard]] QVariantList docks() const;

    [[nodiscard]] QVariantList panels() const;

    [[nodiscard]] static int leastGroupExtent() { return kLeastGroupExtent; }

    Q_INVOKABLE [[nodiscard]] bool isOpen(const QString& panelId) const;

    Q_INVOKABLE void openPanel(const QString& panelId);

    Q_INVOKABLE void closePanel(const QString& panelId);

    Q_INVOKABLE void togglePanel(const QString& panelId);

    Q_INVOKABLE void showPanel(const QString& panelId);

    Q_INVOKABLE void dockPanel(const QString& panelId, Side side, int group);

    Q_INVOKABLE void choosePanel(Side side, int group, int index);

    Q_INVOKABLE void setSideExtent(Side side, int extent);

    Q_INVOKABLE void setGroupExtent(Side side, int group, int extent);

    Q_INVOKABLE void resetWorkspace();

signals:
    void layoutChanged();
    void panelShown(const QString& panelId);

private:
    struct Group {
        QStringList panels;
        int current{0};
        int extent{0};
    };

    struct Dock {
        Side side{Side::Left};
        int extent{0};
        std::vector<Group> groups;
    };

    struct Remembered {
        Side side{Side::Left};
        int group{0};
    };

    [[nodiscard]] Dock& dockOf(Side side) noexcept;
    [[nodiscard]] const Dock& dockOf(Side side) const noexcept;
    [[nodiscard]] static std::size_t placeOfSide(Side side) noexcept;

    void takeOut(const QString& panelId);
    void putIn(const QString& panelId, Side side, int group);
    void tidy();
    void settle();
    void settleQuietly();
    void readLayout();
    void writeLayout() const;
    void layOutAfresh();

    std::vector<Dock> m_docks;
    std::vector<std::pair<QString, Remembered>> m_closed;
};

}
