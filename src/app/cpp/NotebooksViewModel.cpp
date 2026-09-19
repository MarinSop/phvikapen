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
constexpr auto kDraftsFolder = "drafts";
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

QString NotebooksViewModel::draftPathFor(const QString& name) const {
    return m_directory + "/" + kDraftsFolder + "/" + name + kNotebookSuffix;
}

// An open notebook is wherever it already is; a new one starts as a draft.
QString NotebooksViewModel::existingPathFor(const QString& name) const {
    QString kept = pathFor(name);
    if (QFile::exists(kept)) {
        return kept;
    }
    QString draft = draftPathFor(name);
    return QFile::exists(draft) ? std::move(draft) : std::move(kept);
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
    const auto taken = [&name](const QString& existing) {
        return existing.compare(name, Qt::CaseInsensitive) == 0;
    };
    return isUsableName(name) && std::ranges::none_of(m_library, taken)
           && std::ranges::none_of(openNotebooks(), taken) && !QFile::exists(draftPathFor(name));
}

void NotebooksViewModel::createNotebook(const QString& name) {
    createNotebookWithSetup(name, -1, -1, false);
}

void NotebooksViewModel::createNotebookWithSetup(const QString& name, int paper, int background,
                                                 bool landscape, const QUrl& document) {
    if (!isNameFree(name)) {
        emit errorMessage(tr("A notebook called %1 is already there").arg(name));
        return;
    }
    QDir().mkpath(m_directory + "/" + kDraftsFolder);
    openAt(name, draftPathFor(name));
    if (NotebookViewModel* const made = current(); made != nullptr) {
        core::PageStyle wanted = defaults::pageStyle();
        if (paper >= 0 && paper <= static_cast<int>(core::Paper::Custom)) {
            wanted.paper = static_cast<core::Paper>(paper);
        }
        if (background >= 0 && background <= static_cast<int>(core::Background::Dotted)) {
            wanted.background = static_cast<core::Background>(background);
        }
        wanted.orientation = landscape ? core::Orientation::Landscape : core::Orientation::Portrait;
        const auto settle = [wanted, document](NotebookViewModel* const ready) {
            ready->applyStyle(wanted);
            if (!document.isEmpty()) {
                ready->startFromDocument(document);
            }
        };
        if (made->loaded()) {
            settle(made);
        } else {
            const auto connection = std::make_shared<QMetaObject::Connection>();
            *connection =
                connect(made, &NotebookViewModel::loadedChanged, made, [made, settle, connection] {
                    if (!made->loaded()) {
                        return;
                    }
                    QObject::disconnect(*connection);
                    settle(made);
                });
        }
    }
    refreshLibrary();
}

void NotebooksViewModel::openNotebook(const QString& name) {
    openAt(name, existingPathFor(name));
}

void NotebooksViewModel::openAt(const QString& name, const QString& path) {
    if (!isUsableName(name)) {
        emit errorMessage(tr("%1 cannot be used as a name").arg(name));
        return;
    }
    if (const int existing = indexOf(name); existing >= 0) {
        show(existing);
        return;
    }

    auto notebook = std::make_unique<NotebookViewModel>(path, rememberedPage(name), this);
    notebook->setContinuous(m_continuousPages);
    connect(notebook.get(), &NotebookViewModel::notebookPathChanged, this,
            &NotebooksViewModel::openNotebooksChanged);
    connect(notebook.get(), &NotebookViewModel::nameWanted, this,
            [this, kept = notebook.get()](const QString& wanted) { takeName(*kept, wanted); });
    connect(notebook.get(), &NotebookViewModel::saved, this, &NotebooksViewModel::refreshLibrary);
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
    forgetDraftOf(*m_open[position]);
    if (index == m_currentIndex) {
        m_open[position]->setCanvas(nullptr);
    }
    m_open.erase(m_open.begin() + index);
    emit openNotebooksChanged();

    const int wanted = std::min(m_currentIndex, static_cast<int>(m_open.size()) - 1);
    m_currentIndex = -1;
    show(wanted);
}

// The tabs stand in the order the reader puts them in.
// The working copy of a notebook that has been saved elsewhere is of no use once it is closed.
void NotebooksViewModel::forgetDraftOf(const NotebookViewModel& notebook) const {
    const QString working = notebook.notebookPath();
    if (notebook.keptAt().isEmpty() || working != draftPathFor(notebook.name())) {
        return;
    }
    for (const auto* suffix : {"", "-wal", "-shm"}) {
        QFile::remove(working + suffix);
    }
}

bool NotebooksViewModel::isEdited(int index) const {
    if (index < 0 || std::cmp_greater_equal(index, m_open.size())) {
        return false;
    }
    return m_open[static_cast<std::size_t>(index)]->edited();
}

void NotebooksViewModel::moveNotebook(int from, int to) {
    const auto count = static_cast<int>(m_open.size());
    if (from < 0 || from >= count || to < 0 || to >= count || from == to) {
        return;
    }
    const NotebookViewModel* const shown = current();
    std::unique_ptr<NotebookViewModel> carried = std::move(m_open[static_cast<std::size_t>(from)]);
    m_open.erase(m_open.begin() + from);
    m_open.insert(m_open.begin() + to, std::move(carried));
    m_currentIndex = -1;
    emit openNotebooksChanged();
    const auto found =
        std::ranges::find_if(m_open, [shown](const std::unique_ptr<NotebookViewModel>& kept) {
            return kept.get() == shown;
        });
    show(found == m_open.end() ? to : static_cast<int>(std::distance(m_open.begin(), found)));
    rememberSession();
}

// A notebook saved under another name goes by that name from then on.
void NotebooksViewModel::takeName(NotebookViewModel& notebook, const QString& wanted) {
    const QString trimmed = wanted.trimmed();
    if (trimmed == notebook.name() || !isNameFree(trimmed)) {
        return;
    }
    if (notebook.renameTo(pathFor(trimmed))) {
        emit openNotebooksChanged();
        refreshLibrary();
        rememberSession();
    }
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
    const QString folder = QFileInfo{notebook.notebookPath()}.absolutePath();
    if (notebook.renameTo(folder + "/" + trimmed + kNotebookSuffix)) {
        emit openNotebooksChanged();
        refreshLibrary();
        rememberSession();
    }
}

void NotebooksViewModel::deleteNotebook(const QString& name) {
    if (const int open = indexOf(name); open >= 0) {
        closeNotebook(open);
    }
    const QString path = existingPathFor(name);
    if (QFile::exists(path) && !QFile::moveToTrash(path) && !QFile::remove(path)) {
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
    // Drafts are not in the library, but they were open, so they open again.
    names.removeIf([this](const QString& name) { return !QFile::exists(existingPathFor(name)); });
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
