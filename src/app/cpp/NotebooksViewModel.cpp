#include "app/cpp/NotebooksViewModel.hpp"

#include <QStringList>

namespace phvikapen::app {

NotebooksViewModel::NotebooksViewModel(QObject* parent) : QObject(parent) {
    addNotebook();
}

void NotebooksViewModel::addNotebook() {
    QStringList updated = m_titles.value();
    updated.append(tr("Notebook %1").arg(updated.size() + 1));
    m_titles = updated;
}

void NotebooksViewModel::closeNotebook(int index) {
    QStringList updated = m_titles.value();
    if (index < 0 || index >= updated.size()) {
        return;
    }
    updated.removeAt(index);
    m_titles = updated;
}

} // namespace phvikapen::app
