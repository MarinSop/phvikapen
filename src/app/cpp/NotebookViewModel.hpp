#pragma once

#include "app/cpp/OutlineModels.hpp"
#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/undo/UndoStack.hpp"
#include "platform/ink/IInkBackend.hpp"
#include "platform/ink/qt/QtInkItem.hpp"

#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QString>
#include <QtQmlIntegration>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace phvikapen::app {

class NotebookViewModel : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(QString notebookPath READ notebookPath WRITE setNotebookPath NOTIFY
                   notebookPathChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY notebookPathChanged FINAL)
    Q_PROPERTY(QString currentPageId READ currentPageId NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(QString startPage READ startPage WRITE setStartPage NOTIFY startPageChanged FINAL)
    Q_PROPERTY(phvikapen::platform::ink::QtInkItem* canvas READ canvas WRITE setCanvas NOTIFY
                   canvasChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(QString title READ title NOTIFY outlineChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY pageChanged FINAL)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged FINAL)
    Q_PROPERTY(phvikapen::app::OutlineListModel* sections READ sections CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::OutlineListModel* pages READ pages CONSTANT FINAL)
    Q_PROPERTY(int currentSection READ currentSection WRITE setCurrentSection NOTIFY
                   currentPageChanged FINAL)
    Q_PROPERTY(
        int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(int sectionCount READ sectionCount NOTIFY outlineChanged FINAL)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY outlineChanged FINAL)
    Q_PROPERTY(bool hasPreviousPage READ hasPreviousPage NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(bool hasNextPage READ hasNextPage NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Paper paper READ paper WRITE setPaper NOTIFY
                   pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Orientation orientation READ orientation WRITE
                   setOrientation NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Background background READ background WRITE
                   setBackground NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY pageStyleChanged FINAL)

public:
    explicit NotebookViewModel(QObject* parent = nullptr);
    NotebookViewModel(QString path, QString startPage, QObject* parent);
    ~NotebookViewModel() override;

    NotebookViewModel(const NotebookViewModel&) = delete;
    NotebookViewModel& operator=(const NotebookViewModel&) = delete;
    NotebookViewModel(NotebookViewModel&&) = delete;
    NotebookViewModel& operator=(NotebookViewModel&&) = delete;

    void classBegin() override {}

    void componentComplete() override;

    [[nodiscard]] QString notebookPath() const { return m_notebookPath; }

    void setNotebookPath(const QString& path);

    [[nodiscard]] QString name() const;

    [[nodiscard]] QString currentPageId() const;

    [[nodiscard]] QString startPage() const { return m_startPage; }

    void setStartPage(const QString& pageId);

    [[nodiscard]] bool renameTo(const QString& path);

    [[nodiscard]] platform::ink::QtInkItem* canvas() const { return m_canvas; }

    void setCanvas(platform::ink::QtInkItem* canvas);

    [[nodiscard]] bool loaded() const { return m_loaded; }

    [[nodiscard]] QString title() const;

    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

    [[nodiscard]] int strokeCount() const;

    [[nodiscard]] bool canUndo() const { return m_history.canUndo(); }

    [[nodiscard]] bool canRedo() const { return m_history.canRedo(); }

    [[nodiscard]] OutlineListModel* sections() { return &m_sectionsModel; }

    [[nodiscard]] OutlineListModel* pages() { return &m_pagesModel; }

    [[nodiscard]] int currentSection() const;
    void setCurrentSection(int index);
    [[nodiscard]] int currentPage() const;
    void setCurrentPage(int index);
    [[nodiscard]] int sectionCount() const;
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] bool hasPreviousPage() const;
    [[nodiscard]] bool hasNextPage() const;

    [[nodiscard]] page_options::Paper paper() const;
    void setPaper(page_options::Paper paper);
    [[nodiscard]] page_options::Orientation orientation() const;
    void setOrientation(page_options::Orientation orientation);
    [[nodiscard]] page_options::Background background() const;
    void setBackground(page_options::Background background);
    [[nodiscard]] qreal lineSpacing() const;
    void setLineSpacing(qreal millimeters);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearPage();

    Q_INVOKABLE void previousPage();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void addPage();
    Q_INVOKABLE void deletePage(int index);
    Q_INVOKABLE void movePage(int from, int to);
    Q_INVOKABLE void renamePage(int index, const QString& title);

    Q_INVOKABLE void addSection();
    Q_INVOKABLE void deleteSection(int index);
    Q_INVOKABLE void moveSection(int from, int to);
    Q_INVOKABLE void renameSection(int index, const QString& title);

signals:
    void notebookPathChanged();
    void startPageChanged();
    void canvasChanged();
    void loadedChanged();
    void errorMessageChanged();
    void pageChanged();
    void historyChanged();
    void outlineChanged();
    void currentPageChanged();
    void pageStyleChanged();

private:
    class Sink final : public platform::ink::IInkSink {
    public:
        explicit Sink(NotebookViewModel* owner) noexcept : m_owner{owner} {}

        void strokeStarted(const core::InkSample& sample) override;
        void sampleAdded(const core::InkSample& sample) override;
        void strokeFinished(const core::InkSample& sample) override;
        void strokeCompleted(const core::Stroke& stroke) override;
        void strokeCancelled() override;
        void eraserMoved(const core::InkSample& from, const core::InkSample& to) override;
        void eraseFinished() override;

    private:
        NotebookViewModel* m_owner;
    };

    void openNotebook();
    void showLoadedOutline(std::uint64_t opening, core::Result<core::NotebookOutline> outline);
    void showLoadedPage(std::uint64_t opening, const core::Uuid& pageId,
                        core::Result<std::vector<core::PlacedStroke>> strokes);
    void goToPage(const core::Uuid& pageId);
    void goToPlace(std::size_t section, std::size_t page);
    void setLoaded(bool loaded);

    [[nodiscard]] core::Page* currentPageData();
    [[nodiscard]] const core::Page* currentPageData() const;
    [[nodiscard]] const core::PageInfo* currentPageInfo() const;
    [[nodiscard]] std::optional<core::PagePlace> currentPlace() const;
    [[nodiscard]] std::optional<std::size_t> currentSectionIndex() const;
    void changeStyle(const core::PageStyle& style);

    void storeStroke(const core::Stroke& stroke);
    void erase(const core::InkSample& from, const core::InkSample& to);
    void finishErasing();
    void runCommand(std::unique_ptr<core::ICommand> command);
    void finishChange(const core::Result<void>& change, std::optional<core::Uuid> pageToShow);
    void publishOutline();
    void refreshCanvas();
    void reportError(const QString& message);

    Sink m_sink{this};
    core::Uuid7Generator m_ids;
    core::Outline m_outline;
    std::map<core::Uuid, std::unique_ptr<core::Page>> m_pages;
    std::optional<core::StorageThread> m_storage;
    core::UndoStack m_history;
    std::vector<core::Uuid> m_erasing;
    core::Uuid m_currentPage;
    OutlineListModel m_sectionsModel;
    OutlineListModel m_pagesModel;
    QPointer<platform::ink::QtInkItem> m_canvas;
    QString m_notebookPath;
    QString m_startPage;
    QString m_errorMessage;
    std::uint64_t m_opening{0};
    bool m_completed{false};
    bool m_loaded{false};
};

}
