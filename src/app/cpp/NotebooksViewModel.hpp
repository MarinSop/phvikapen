#pragma once

#include <QObject>
#include <QProperty>
#include <QStringList>
#include <QtQmlIntegration>

namespace phvikapen::app {

/// The notebooks that are currently open, one per tab.
///
/// TODO(M3): Back this with notebooks on disk instead of in-memory placeholders.
class NotebooksViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QStringList titles READ titles NOTIFY titlesChanged BINDABLE bindableTitles FINAL)

public:
    explicit NotebooksViewModel(QObject* parent = nullptr);

    [[nodiscard]] QStringList titles() const { return m_titles.value(); }

    [[nodiscard]] QBindable<QStringList> bindableTitles() { return {&m_titles}; }

    /// Opens another placeholder notebook.
    Q_INVOKABLE void addNotebook();

    /// Closes the notebook at @p index, ignoring an index outside the list.
    Q_INVOKABLE void closeNotebook(int index);

signals:
    void titlesChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(NotebooksViewModel, QStringList, m_titles,
                               &NotebooksViewModel::titlesChanged)
};

} // namespace phvikapen::app
