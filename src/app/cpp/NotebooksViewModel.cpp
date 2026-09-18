#include "app/cpp/NotebooksViewModel.hpp"

#include "app/cpp/NotebookViewModel.hpp"
#include "app/cpp/PageDefaults.hpp"
#include "core/model/PageStyle.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

namespace phvikapen::app {
namespace {

constexpr auto kNotebookSuffix = ".phvika";
constexpr auto kFirstNotebookName = "Notebook";
constexpr auto kOpenSetting = "notebooks/open";
constexpr auto kCurrentSetting = "notebooks/current";
constexpr auto kPageSettingPrefix = "notebooks/page/";

[[nodiscard]] QString defaultDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/notebooks";
}

[[nodiscard]] bool isUsableName(const QString& name) {
    return !name.isEmpty() && !name.contains('/') && !name.contains('\\') && !name.contains(':')
           && name != "." && name != "..";
}

}

NotebooksViewModel::NotebooksViewModel(QObject* parent)
    : QObject(parent), m_directory{defaultDirectory()} {}

void NotebooksViewModel::componentComplete() {
    m_completed = true;
    refreshLibrary();
    restoreSession();
}

void NotebooksViewModel::setDirectory(const QString& directory) {
    if (m_directory == directory) {
        return;
    }
    m_directory = directory;
    emit directoryChanged();
    if (!m_completed) {
        return;
    }
    while (!m_open.empty()) {
        closeNotebook(static_cast<int>(m_open.size()) - 1);
    }
    refreshLibrary();
    restoreSession();
}

QString NotebooksViewModel::pathFor(const QString& name) const {
    return m_directory + "/" + name + kNotebookSuffix;
}

void NotebooksViewModel::setContinuousPages(bool continuous) {
    if (continuous == m_continuousPages) {
        return;
    }
    m_continuousPages = continuous;
    for (const std::unique_ptr<NotebookViewModel>& notebook : m_open) {
        notebook->setContinuous(continuous);
    }
    emit continuousPagesChanged();
}

void NotebooksViewModel::refreshLibrary() {
    QDir directory{m_directory};
    if (!directory.exists() && !QDir().mkpath(m_directory)) {
        emit errorMessage(tr("Could not create %1").arg(m_directory));
        return;
    }
    QStringList found;
    const QString pattern = QString{"*"} + kNotebookSuffix;
    for (const QFileInfo& file :
         directory.entryInfoList({pattern}, QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        found.append(file.completeBaseName());
    }
    for (const QString& name : openNotebooks()) {
        if (!found.contains(name, Qt::CaseInsensitive)) {
            found.append(name);
        }
    }
    std::ranges::sort(found, [](const QString& first, const QString& second) {
        return first.compare(second, Qt::CaseInsensitive) < 0;
    });
    if (found != m_library) {
        m_library = found;
        emit libraryChanged();
    }
}

QStringList NotebooksViewModel::openNotebooks() const {
    QStringList names;
    names.reserve(static_cast<qsizetype>(m_open.size()));
    for (const std::unique_ptr<NotebookViewModel>& notebook : m_open) {
        names.append(notebook->name());
    }
    return names;
}

NotebookViewModel* NotebooksViewModel::current() const {
    if (m_currentIndex < 0 || std::cmp_greater_equal(m_currentIndex, m_open.size())) {
        return nullptr;
    }
    return m_open[static_cast<std::size_t>(m_currentIndex)].get();
}

int NotebooksViewModel::indexOf(const QString& name) const {
    const QStringList names = openNotebooks();
    return static_cast<int>(names.indexOf(name));
}

void NotebooksViewModel::setCanvas(platform::ink::QtInkItem* canvas) {
    if (m_canvas == canvas) {
        return;
    }
    if (NotebookViewModel* const notebook = current()) {
        notebook->setCanvas(nullptr);
    }
    m_canvas = canvas;
    emit canvasChanged();
    if (NotebookViewModel* const notebook = current()) {
        notebook->setCanvas(m_canvas);
    }
}

void NotebooksViewModel::show(int index) {
    if (index == m_currentIndex) {
        return;
    }
    if (NotebookViewModel* const previous = current()) {
        rememberPage(*previous);
        previous->setCanvas(nullptr);
    }
    m_currentIndex = index;
    if (NotebookViewModel* const notebook = current()) {
        notebook->setCanvas(m_canvas);
    }
    emit currentChanged();
    rememberSession();
}

void NotebooksViewModel::setCurrentIndex(int index) {
    if (index >= 0 && std::cmp_less(index, m_open.size())) {
        show(index);
    }
}

QString NotebooksViewModel::suggestedName() const {
    if (isNameFree(kFirstNotebookName)) {
        return kFirstNotebookName;
    }
    for (int number = 2;; ++number) {
        const QString candidate = QString{"%1 %2"}.arg(kFirstNotebookName).arg(number);
        if (isNameFree(candidate)) {
            return candidate;
        }
    }
}

bool NotebooksViewModel::isNameFree(const QString& name) const {
    return isUsableName(name) && std::ranges::none_of(m_library, [&name](const QString& existing) {
               return existing.compare(name, Qt::CaseInsensitive) == 0;
           });
}

void NotebooksViewModel::createNotebook(const QString& name) {
    createNotebookWithSetup(name, -1, -1, false);
}

void NotebooksViewModel::createNotebookWithSetup(const QString& name, int paper, int background,
                                                 bool landscape) {
    if (!isNameFree(name)) {
        emit errorMessage(tr("A notebook called %1 is already there").arg(name));
        return;
    }
    openNotebook(name);
    if (NotebookViewModel* const made = current(); made != nullptr) {
        core::PageStyle wanted = defaults::pageStyle();
        if (paper >= 0 && paper <= static_cast<int>(core::Paper::Custom)) {
            wanted.paper = static_cast<core::Paper>(paper);
        }
        if (background >= 0 && background <= static_cast<int>(core::Background::Dotted)) {
            wanted.background = static_cast<core::Background>(background);
        }
        wanted.orientation = landscape ? core::Orientation::Landscape : core::Orientation::Portrait;
        if (made->loaded()) {
            made->applyStyle(wanted);
        } else {
            const auto connection = std::make_shared<QMetaObject::Connection>();
            *connection =
                connect(made, &NotebookViewModel::loadedChanged, made, [made, wanted, connection] {
                    if (!made->loaded()) {
                        return;
                    }
                    QObject::disconnect(*connection);
                    made->applyStyle(wanted);
                });
        }
    }
    refreshLibrary();
}

void NotebooksViewModel::openNotebook(const QString& name) {
    if (!isUsableName(name)) {
        emit errorMessage(tr("%1 cannot be used as a name").arg(name));
        return;
    }
    if (const int existing = indexOf(name); existing >= 0) {
        show(existing);
        return;
    }

    auto notebook = std::make_unique<NotebookViewModel>(pathFor(name), rememberedPage(name), this);
    notebook->setContinuous(m_continuousPages);
    connect(notebook.get(), &NotebookViewModel::notebookPathChanged, this,
            &NotebooksViewModel::openNotebooksChanged);
    m_open.push_back(std::move(notebook));
    emit openNotebooksChanged();
    m_currentIndex = -1;
    show(static_cast<int>(m_open.size()) - 1);
    refreshLibrary();
}

void NotebooksViewModel::closeNotebook(int index) {
    if (index < 0 || std::cmp_greater_equal(index, m_open.size())) {
        return;
    }
    const auto position = static_cast<std::size_t>(index);
    rememberPage(*m_open[position]);
    if (index == m_currentIndex) {
        m_open[position]->setCanvas(nullptr);
    }
    m_open.erase(m_open.begin() + index);
    emit openNotebooksChanged();

    const int wanted = std::min(m_currentIndex, static_cast<int>(m_open.size()) - 1);
    m_currentIndex = -1;
    show(wanted);
}

void NotebooksViewModel::renameNotebook(int index, const QString& name) {
    if (index < 0 || std::cmp_greater_equal(index, m_open.size())) {
        return;
    }
    const QString trimmed = name.trimmed();
    NotebookViewModel& notebook = *m_open[static_cast<std::size_t>(index)];
    if (trimmed == notebook.name()) {
        return;
    }
    if (!isNameFree(trimmed)) {
        emit errorMessage(tr("A notebook called %1 is already there").arg(trimmed));
        return;
    }
    if (notebook.renameTo(pathFor(trimmed))) {
        emit openNotebooksChanged();
        refreshLibrary();
        rememberSession();
    }
}

void NotebooksViewModel::deleteNotebook(const QString& name) {
    if (const int open = indexOf(name); open >= 0) {
        closeNotebook(open);
    }
    const QString path = pathFor(name);
    if (!QFile::moveToTrash(path) && !QFile::remove(path)) {
        emit errorMessage(tr("Could not delete %1").arg(name));
        return;
    }
    for (const auto* suffix : {"-wal", "-shm"}) {
        QFile::remove(path + suffix);
    }
    refreshLibrary();
}

void NotebooksViewModel::restoreSession() {
    m_restoring = true;
    const QSettings settings;
    QStringList names = settings.value(kOpenSetting).toStringList();
    names.removeIf([this](const QString& name) { return !m_library.contains(name); });
    for (const QString& name : names) {
        openNotebook(name);
    }

    const QString wanted = settings.value(kCurrentSetting).toString();
    const int index = indexOf(wanted);
    m_restoring = false;
    if (index >= 0) {
        show(index);
    }
    rememberSession();
}

void NotebooksViewModel::rememberSession() const {
    if (!m_completed || m_restoring) {
        return;
    }
    QSettings settings;
    settings.setValue(kOpenSetting, openNotebooks());
    if (const NotebookViewModel* const notebook = current()) {
        settings.setValue(kCurrentSetting, notebook->name());
    }
}

void NotebooksViewModel::rememberPage(const NotebookViewModel& notebook) {
    const QString page = notebook.currentPageId();
    if (!page.isEmpty()) {
        QSettings settings;
        settings.setValue(kPageSettingPrefix + notebook.name(), page);
    }
}

QString NotebooksViewModel::rememberedPage(const QString& name) {
    const QSettings settings;
    return settings.value(kPageSettingPrefix + name).toString();
}

}
